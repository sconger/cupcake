
#ifdef _WIN32

#include "cupcake/net/Socket.h"

#include "cupcake/internal/Global_win.h"
#include "cupcake/internal/net/SocketImpl_win.h"

#include <Ws2tcpip.h>
#include <Mstcpip.h>

#include <experimental/resumable>

using namespace Cupcake;

// For whatever reason, calls like AcceptEx and ConnectEx giver non-WSA error
// values after you wait for their completion. Just added them to the switch.
static SocketError getSocketError(int errVal) {
    switch (errVal) {
    case WSANOTINITIALISED:
    case WSAEINVALIDPROVIDER:
    case WSAEINVALIDPROCTABLE:
        return SocketError::NotInitialized;
    case WSAENETDOWN:
        return SocketError::NetworkDown;
    case WSAEADDRINUSE:
        return SocketError::AddressInUse;
    case WSAEACCES:
        return SocketError::AccessDenied;
    case WSAEALREADY:
        return SocketError::ConnectionAlreadyInProgress;
    case WSAEADDRNOTAVAIL:
        return SocketError::AddressNotAvailable;
    case WSAEOPNOTSUPP:
    case WSAEAFNOSUPPORT:
    case WSAEPROTONOSUPPORT:
    case WSAESOCKTNOSUPPORT:
        return SocketError::NotSupported;
    case WSAECONNREFUSED:
    case ERROR_CONNECTION_REFUSED:
        return SocketError::ConnectionRefused;
    case WSAEFAULT:
        return SocketError::BadAddress;
    case WSAEINVAL:
    case WSAEPROTOTYPE:
    case ERROR_INVALID_NETNAME: // Invalid socket addr
    case WSAENOPROTOOPT: // Bad setsocketopt argument
        return SocketError::InvalidArgument;
    case WSAEISCONN:
        return SocketError::AlreadyConnected;
    case WSAENETUNREACH:
    case ERROR_NETWORK_UNREACHABLE:
        return SocketError::NetworkUnreachable;
    case WSAEHOSTUNREACH:
    case ERROR_HOST_UNREACHABLE:
        return SocketError::HostUnreachable;
    case WSAENOBUFS:
        return SocketError::NoBufferSpace;
    case WSAENOTSOCK:
    case ERROR_INVALID_HANDLE:
        return SocketError::InvalidHandle;
    case WSAEMFILE:
        return SocketError::TooManyHandles;
    case WSAETIMEDOUT:
    case ERROR_SEM_TIMEOUT: // Timeout
        return SocketError::TimedOut;
    case WSAECONNABORTED:
        return SocketError::ConnectionAborted;
    case WSAECONNRESET:
        return SocketError::ConnectionReset;
    case WSAEMSGSIZE:
        return SocketError::MessageTooLong;
    case WSAENETRESET:
        return SocketError::NetworkReset;
    case WSAENOTCONN:
        return SocketError::NotConnected;
    case WSAESHUTDOWN:
        return SocketError::ConnectionShutdown;
    case ERROR_OPERATION_ABORTED: // Socket close
        return SocketError::OperationAborted;
    default:
        return SocketError::Unknown;
    }
}

class CompletionResult {
public:
    CompletionResult() : bytesTransfered(0), error(0) {}
    ULONG_PTR bytesTransfered;
    DWORD error;
};

/*
 * Checks if this is an AcceptEx error where we should just retry.
 */
static
bool isAcceptExRetryError(int error) {
    switch (error) {
    case WSAECONNRESET:
    case ERROR_NETNAME_DELETED:
        return true;
    default:
        return false;
    }
}

/*
 * Calls AcceptEx with a retry loop based on the immediate error.
 */
static
int acceptExWithRetry(SOCKET socket,
                      PTP_IO ptpIo,
                      SOCKET preparedSocket,
                      char* addrBuffer,
                      OVERLAPPED* overlappedData) {
    while (true) {
        ::StartThreadpoolIo(ptpIo);

        BOOL res = Global::getAcceptEx()(socket,
            preparedSocket,
            addrBuffer,
            0,
            sizeof(sockaddr_storage) + 16,
            sizeof(sockaddr_storage) + 16,
            NULL,
            overlappedData);

        if (res) {
            ::CancelThreadpoolIo(ptpIo);
            return ERROR_SUCCESS;
        } else {
            int wsaErr = ::WSAGetLastError();

            if (!isAcceptExRetryError(wsaErr)) {
                if (wsaErr != WSA_IO_PENDING) {
                    ::CancelThreadpoolIo(ptpIo);
                }
                return wsaErr;
            }
        }
    }
}

// Note that while it would be nice to use inheritance for this, the classic method won't
// work due to needing to start with an OVERLAPPED (and not a vtable).
class OverlappedData {
public:
    OverlappedData() :
        coroutineHandle(nullptr),
        completionResult(nullptr),
        isAccept(false) {
        std::memset(&overlapped, 0, sizeof(OVERLAPPED));
    }

    OverlappedData(SOCKET socket,
                   PTP_IO ptpIo,
                   SOCKET preparedSocket,
                   char* addrBuffer) :
        coroutineHandle(nullptr),
        completionResult(nullptr),
        isAccept(true),
        socket(socket),
        ptpIo(ptpIo),
        preparedSocket(preparedSocket),
        addrBuffer(addrBuffer) {
        std::memset(&overlapped, 0, sizeof(OVERLAPPED));
    }

    void handleCompletion(ULONG ioResult, ULONG_PTR numberOfBytesTransferred) {
        // Handle AcceotEx errors that indicate the other side hung up by retrying.
        // No reason to expose to users.
        if (isAccept &&
            isAcceptExRetryError((int)ioResult)) {

            int acceptExErr = acceptExWithRetry(socket,
                ptpIo,
                preparedSocket,
                addrBuffer,
                (OVERLAPPED*)this);

            completionResult->error = (ULONG)acceptExErr;
            completionResult->bytesTransfered = 0;
        } else {
            completionResult->error = ioResult;
            completionResult->bytesTransfered = numberOfBytesTransferred;
        }

        std::experimental::coroutine_handle<> convertedHandle =
            std::experimental::coroutine_handle<>::from_address(coroutineHandle);
        convertedHandle.resume();
    }

    // Needs to start with an OVERLAPPED
    OVERLAPPED overlapped;

    // Generic fields
    void* coroutineHandle;
    CompletionResult* completionResult;
    bool isAccept;

    // AcceptEx retry data
    SOCKET socket;
    PTP_IO ptpIo;
    SOCKET preparedSocket;
    char* addrBuffer;
};

static
void CALLBACK completionCallback(PTP_CALLBACK_INSTANCE instance,
    PVOID context,
    PVOID overlapped,
    ULONG ioResult,
    ULONG_PTR numberOfBytesTransferred,
    PTP_IO ptpIo) {

    OverlappedData* data = (OverlappedData*)overlapped;
    data->handleCompletion(ioResult, numberOfBytesTransferred);
}

static
SocketError initSocket(SOCKET* pSocket, PTP_IO* pPtpIo, int family) {
    (*pSocket) = INVALID_SOCKET;
    (*pPtpIo) = NULL;

    // Create the socket
    DWORD flags = WSA_FLAG_OVERLAPPED;
#ifdef WSA_FLAG_NO_HANDLE_INHERIT
    flags |= WSA_FLAG_NO_HANDLE_INHERIT;
#endif

    SOCKET socket = ::WSASocketW(family,
        SOCK_STREAM,
        IPPROTO_TCP,
        NULL,
        0,
        flags);

    if (socket == INVALID_SOCKET) {
        int wsaErr = ::WSAGetLastError();
        return getSocketError(wsaErr);
    }

    // Allow immediate completion
    BOOL modeRes = ::SetFileCompletionNotificationModes((HANDLE)socket,
        FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
    if (!modeRes) {
        ::closesocket(socket);
        return SocketError::Unknown;
    }

    // For IPv6 sockets, disable IPv6 only mode
    /*
    if (family == AF_INET6) {
    DWORD optVal = 0;
    int optRes = ::setsockopt(socket,
    IPPROTO_IPV6,
    IPV6_V6ONLY,
    (char*)&optVal,
    sizeof(optVal));

    if (optRes != 0) {
    int wsaErr = ::WSAGetLastError();
    ::closesocket(socket);
    return getSetsockoptError(wsaErr);
    }
    }
    */

    // Turn off Nagle's algorithm
    DWORD noDelayOpt = 1;
    int setOptRes = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelayOpt, sizeof(DWORD));
    if (setOptRes != 0) {
        int wsaErr = ::WSAGetLastError();
        ::closesocket(socket);
        return getSocketError(wsaErr);
    }

    // Create a thread pool IO for the handle
    PTP_IO ptpIo = ::CreateThreadpoolIo((HANDLE)socket,
        completionCallback,
        NULL,
        NULL);
    if (ptpIo == NULL) {
        ::closesocket(socket);
        return SocketError::Unknown;
    }

    (*pSocket) = socket;
    (*pPtpIo) = ptpIo;
    return SocketError::Ok;
}

// General TODO: Need to re-submit if WSAECONNRESET and the like
class SocketImpl::AcceptAwaiter {
public:
    AcceptAwaiter(SocketImpl* socketImpl,
                  SOCKET preparedSocket,
                  PTP_IO preparedPtpIo) :
        socketImpl(socketImpl),
        preparedSocket(preparedSocket),
        preparedPtpIo(preparedPtpIo),
        overlappedData(socketImpl->socket,
                       socketImpl->ptpIo,
                       preparedSocket,
                       addrBuffer),
        newSocket(nullptr),
        socketError(SocketError::Ok) {}

    bool await_ready() const {
        return false;
    }

    bool await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        overlappedData.coroutineHandle = coroutineHandle.to_address();
        overlappedData.completionResult = &completionResult;

        // Trigger AcceptEx
        int acceptExErr = acceptExWithRetry(socketImpl->socket,
            socketImpl->ptpIo,
            preparedSocket,
            addrBuffer,
            (OVERLAPPED*)&overlappedData);

        if (acceptExErr == ERROR_SUCCESS) {
            onAccept();
            return false;
        } else if (acceptExErr != WSA_IO_PENDING) {
            socketError = getSocketError(acceptExErr);
            return false;
        }

        return true;
    }

    std::tuple<Socket, SocketError> await_resume() {
        if (socketError != SocketError::Ok) {
            return std::make_tuple(Socket(), socketError);
        } else if (completionResult.error != ERROR_SUCCESS) {
            return std::make_tuple(Socket(), getSocketError((int)completionResult.error));
        } else {
            onAccept();
            return std::make_tuple(Socket(newSocket), SocketError::Ok);
        }
    }

private:
    /* Helper that does the work needed after an accept succeeds */
    void onAccept() {
        // Have to set the SO_UPDATE_ACCEPT_CONTEXT (see AcceptEx doc)
        int setSockRes = ::setsockopt(preparedSocket,
            SOL_SOCKET,
            SO_UPDATE_ACCEPT_CONTEXT,
            (char *)&preparedSocket,
            sizeof(SOCKET));

        if (setSockRes != 0) {
            int wsaErr = ::WSAGetLastError();
            socketError = getSocketError(wsaErr);
            return;
        }

        // Parse out the address information
        SOCKADDR* localPtr = nullptr;
        INT localLen = 0;
        SOCKADDR* remotePtr = nullptr;
        INT remoteLen = 0;

        Global::getGetAcceptExSockaddrs()(addrBuffer,
            0,
            sizeof(sockaddr_storage) + 16,
            sizeof(sockaddr_storage) + 16,
            &localPtr,
            &localLen,
            &remotePtr,
            &remoteLen);

        // Make a happy new Socket object
        newSocket = new SocketImpl();
        newSocket->socket = preparedSocket;
        newSocket->ptpIo = preparedPtpIo;
        // TODO: Use length values
        newSocket->localAddr = SockAddr::fromNative((const sockaddr_storage*)localPtr);
        newSocket->remoteAddr = SockAddr::fromNative((const sockaddr_storage*)remotePtr);
    }

    SocketImpl* socketImpl;
    SOCKET preparedSocket;
    PTP_IO preparedPtpIo;
    OverlappedData overlappedData;
    CompletionResult completionResult;

    char addrBuffer[2 * (sizeof(sockaddr_storage) + 16)];

    // Result values
    SocketImpl* newSocket;
    SocketError socketError;
};

class SocketImpl::ConnectAwaiter {
public:
    ConnectAwaiter(SocketImpl* socketImpl,
        const SockAddr& sockAddr) :
        socketImpl(socketImpl),
        overlappedData(),
        sockAddr(sockAddr),
        socketError(SocketError::Ok) {}

    bool await_ready() const {
        return false;
    }

    bool await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        overlappedData.coroutineHandle = coroutineHandle.to_address();
        overlappedData.completionResult = &completionResult;

        sockaddr_storage storage;
        sockAddr.toNative(&storage);

        ::StartThreadpoolIo(socketImpl->ptpIo);

        BOOL res = Global::getConnectEx()(socketImpl->socket,
            (const sockaddr*)&storage,
            sizeof(sockaddr_storage),
            NULL,
            0,
            NULL,
            (OVERLAPPED*)&overlappedData);

        if (res) {
            ::CancelThreadpoolIo(socketImpl->ptpIo);
            onConnect();
            return false;
        } else {
            int wsaErr = ::WSAGetLastError();

            if (wsaErr != WSA_IO_PENDING) {
                ::CancelThreadpoolIo(socketImpl->ptpIo);
                socketError = getSocketError(wsaErr);
                return false;
            }
        }

        return true;
    }

    SocketError await_resume() {
        if (socketError != SocketError::Ok) {
            return socketError;
        } else if (completionResult.error != ERROR_SUCCESS) {
            return getSocketError((int)completionResult.error);
        } else {
            onConnect();
            return SocketError::Ok;
        }
    }

private:
    /* Helper that does the work needed after a connect succeeds */
    void onConnect() {
        sockaddr_storage storage;
        int nameLen = sizeof(sockaddr_storage);
        int nameRes = ::getsockname(socketImpl->socket, (sockaddr*)&storage, &nameLen);
        if (nameRes != 0) {
            socketError = getSocketError(::WSAGetLastError());
            return;
        }

        // Copy in the remote address
        socketImpl->remoteAddr.fromNative(&storage);
    }

    SocketImpl* socketImpl;
    OverlappedData overlappedData;
    const SockAddr& sockAddr;
    CompletionResult completionResult;

    // Result value
    SocketError socketError;
};

class SocketImpl::ReadAwaiter {
public:
    ReadAwaiter(SocketImpl* socketImpl,
                INet::IoBuffer* buffers,
                uint32_t bufferCount) :
        socketImpl(socketImpl),
        buffers(buffers),
        bufferCount(bufferCount),
        overlappedData(),
        socketError(SocketError::Ok),
        bytesXfer(0)
    {}

    bool await_ready() const {
        return false;
    }

    bool await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        overlappedData.coroutineHandle = coroutineHandle.to_address();
        overlappedData.completionResult = &completionResult;

        DWORD flags = 0;
        DWORD bytesRecv = 0;

        WSABUF* wsaBufs = (WSABUF*)_malloca(bufferCount * sizeof(WSABUF));
        if (!wsaBufs) {
            throw std::bad_alloc();
        }
        for (size_t i = 0; i < bufferCount; i++) {
            wsaBufs[i].buf = buffers[i].buffer;
            wsaBufs[i].len = buffers[i].bufferLen;
        }

        ::StartThreadpoolIo(socketImpl->ptpIo);

        int res = ::WSARecv(socketImpl->socket,
            wsaBufs,
            bufferCount,
            &bytesRecv,
            &flags,
            (OVERLAPPED*)&overlappedData,
            NULL);

        if (res == 0) {
            ::CancelThreadpoolIo(socketImpl->ptpIo);
            bytesXfer = bytesRecv;
            _freea(wsaBufs);
            return false;
        } else {
            int wsaErr = ::WSAGetLastError();

            if (wsaErr != WSA_IO_PENDING) {
                ::CancelThreadpoolIo(socketImpl->ptpIo);
                socketError = getSocketError(wsaErr);
                _freea(wsaBufs);
                return false;
            }
        }

        _freea(wsaBufs);
        return true;
    }

    std::tuple<uint32_t, SocketError> await_resume() {
        if (socketError != SocketError::Ok) {
            return std::make_tuple(0, socketError);
        } else if (bytesXfer) {
            return std::make_tuple(bytesXfer, SocketError::Ok);
        } else if (completionResult.error != ERROR_SUCCESS) {
            return std::make_tuple(0, getSocketError((int)completionResult.error));
        } else {
            return std::make_tuple((uint32_t)completionResult.bytesTransfered, SocketError::Ok);
        }
    }

private:
    SocketImpl* socketImpl;
    INet::IoBuffer* buffers;
    uint32_t bufferCount;
    OverlappedData overlappedData;
    CompletionResult completionResult;

    // Result value
    SocketError socketError;
    uint32_t bytesXfer;
};

class SocketImpl::WriteAwaiter {
public:
    WriteAwaiter(SocketImpl* socketImpl,
        const INet::IoBuffer* buffers,
        uint32_t bufferCount) :
        socketImpl(socketImpl),
        buffers(buffers),
        bufferCount(bufferCount),
        overlappedData(),
        socketError(SocketError::Ok),
        bytesXfer(0)
    {}

    bool await_ready() const {
        return false;
    }

    bool await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        overlappedData.coroutineHandle = coroutineHandle.to_address();
        overlappedData.completionResult = &completionResult;

        DWORD bytesSent;

        WSABUF* wsaBufs = (WSABUF*)_malloca(bufferCount * sizeof(WSABUF));
        if (!wsaBufs) {
            throw std::bad_alloc();
        }
        for (size_t i = 0; i < bufferCount; i++) {
            wsaBufs[i].buf = buffers[i].buffer;
            wsaBufs[i].len = buffers[i].bufferLen;
        }

        ::StartThreadpoolIo(socketImpl->ptpIo);

        int res = ::WSASend(socketImpl->socket,
            wsaBufs,
            bufferCount,
            &bytesSent,
            0,
            (OVERLAPPED*)&overlappedData,
            NULL);

        if (res == 0) {
            ::CancelThreadpoolIo(socketImpl->ptpIo);
            bytesXfer = bytesSent;
            _freea(wsaBufs);
            return false;
        } else {
            int wsaErr = ::WSAGetLastError();

            if (wsaErr != WSA_IO_PENDING) {
                ::CancelThreadpoolIo(socketImpl->ptpIo);
                socketError = getSocketError(wsaErr);
                _freea(wsaBufs);
                return false;
            }
        }

        _freea(wsaBufs);
        return true;
    }

    SocketError await_resume() {
        if (socketError != SocketError::Ok) {
            return socketError;
        } else if (bytesXfer) {
            return SocketError::Ok;
        } else if (completionResult.error != ERROR_SUCCESS) {
            return getSocketError((int)completionResult.error);
        } else {
            return SocketError::Ok;
        }
    }

private:
    SocketImpl* socketImpl;
    const INet::IoBuffer* buffers;
    uint32_t bufferCount;
    OverlappedData overlappedData;
    CompletionResult completionResult;

    // Result value
    SocketError socketError;
    uint32_t bytesXfer;
};

SocketImpl::SocketImpl() :
    socket(INVALID_SOCKET),
    ptpIo(NULL),
    localAddr(),
    remoteAddr()
{}

SocketImpl::~SocketImpl() {
    close();
}

SocketError SocketImpl::init(INet::Protocol prot) {
    if (socket != INVALID_SOCKET) {
        return SocketError::InvalidState;
    }

    // Make sure global data needed by this is initialized
    Global::initGlobals();

    int family;

    if (prot == INet::Protocol::Ipv4) {
        family = AF_INET;
    } else if (prot == INet::Protocol::Ipv6) {
        family = AF_INET6;
    } else {
        return SocketError::InvalidArgument;
    }

    return initSocket(&socket, &ptpIo, family);
}

SocketError SocketImpl::close() {
    if (socket != INVALID_SOCKET) {
        int closeRes = ::closesocket(socket);
        socket = INVALID_SOCKET;
        if (closeRes == SOCKET_ERROR) {
            return getSocketError(::WSAGetLastError());
        }
    }

    return SocketError::Ok;
}

SockAddr SocketImpl::getLocalAddress() const {
    return localAddr;
}

SockAddr SocketImpl::getRemoteAddress() const {
    return remoteAddr;
}

SocketError SocketImpl::bind(const SockAddr& sockAddr) {
    if (socket == INVALID_SOCKET) {
        return SocketError::NotInitialized;
    }

    sockaddr_storage storage;
    sockAddr.toNative(&storage);

    // For bind we also want to set the SO_EXCLUSIVEADDRUSE option
    DWORD opt = 1;
    int optRes = ::setsockopt(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&opt, sizeof(DWORD));
    if (optRes != 0) {
        return getSocketError(::WSAGetLastError());
    }

    int bindRes = ::bind(socket, (const sockaddr*)&storage, sizeof(sockaddr_storage));
    if (bindRes != 0) {
        return getSocketError(::WSAGetLastError());
    }

    // And then we need to fetch back the bound addr/port
    int nameLen = sizeof(sockaddr_storage);
    int nameRes = ::getsockname(socket, (sockaddr*)&storage, &nameLen);
    if (nameRes != 0) {
        return getSocketError(::WSAGetLastError());
    }

    localAddr = SockAddr::fromNative(&storage);

    return SocketError::Ok;
}

SocketError SocketImpl::listen() {
    return listen(SOMAXCONN);
}

SocketError SocketImpl::listen(uint32_t queue) {
    if (socket == INVALID_SOCKET) {
        return SocketError::NotInitialized;
    }

    if (queue > SOMAXCONN) {
        queue = SOMAXCONN;
    }

    int res = ::listen(socket, (int)queue);
    if (res != 0) {
        return getSocketError(::WSAGetLastError());
    }

    return SocketError::Ok;
}

std::future<void> SocketImpl::accept_co(SOCKET preparedSocket, PTP_IO preparedPtpIo, std::tuple<Socket, SocketError>* res) {
    (*res) = co_await AcceptAwaiter(this, preparedSocket, preparedPtpIo);
}

std::tuple<Socket, SocketError> SocketImpl::accept() {
    if (socket == INVALID_SOCKET) {
        return std::make_tuple(Socket(), SocketError::NotInitialized);
    }

    SOCKET preparedSocket;
    PTP_IO preparedPtpIo;

    int family;

    if (localAddr.getFamily() == INet::Protocol::Ipv4) {
        family = AF_INET;
    } else if (localAddr.getFamily() == INet::Protocol::Ipv6) {
        family = AF_INET6;
    } else {
        return std::make_tuple(Socket(), SocketError::InvalidArgument);
    }

    // Initialize a socket to pass to AcceptEx
    SocketError err = initSocket(&preparedSocket, &preparedPtpIo, family);
    if (err != SocketError::Ok) {
        return std::make_tuple(Socket(), err);
    }

    std::tuple<Socket, SocketError> res(Socket(), SocketError::Ok);
    accept_co(preparedSocket, preparedPtpIo, &res).get();
    return std::move(res);
}

std::future<void> SocketImpl::connect_co(const SockAddr& sockAddr, SocketError* res) {
    (*res) = co_await ConnectAwaiter(this, sockAddr);
}

SocketError SocketImpl::connect(const SockAddr& sockAddr) {
    if (socket == INVALID_SOCKET) {
        return SocketError::NotInitialized;
    }

    sockaddr_storage storage;
    sockAddr.toNative(&storage);

    // For reasons that I presume have to do with DisconnectEx, ConnectEx
    // requires a bound socket
    sockaddr_storage addrAny;
    addrAny.ss_family = storage.ss_family;
    INETADDR_SETANY((sockaddr*)&addrAny);

    int bindRes = ::bind(socket, (const sockaddr*)&addrAny, sizeof(sockaddr_storage));
    if (bindRes != 0) {
        return getSocketError(::WSAGetLastError());
    }

    SocketError res;
    connect_co(sockAddr, &res).get();
    return res;
}

std::future<void> SocketImpl::read_co(INet::IoBuffer* buffers, uint32_t bufferCount, std::tuple<uint32_t, SocketError>* res) {
    (*res) = co_await ReadAwaiter(this, buffers, bufferCount);
}

std::tuple<uint32_t, SocketError> SocketImpl::read(char* buffer, uint32_t bufferLen) {
    INet::IoBuffer ioBuffer;
    ioBuffer.buffer = buffer;
    ioBuffer.bufferLen = bufferLen;
    return readv(&ioBuffer, 1);
}

std::tuple<uint32_t, SocketError> SocketImpl::readv(INet::IoBuffer* buffers, uint32_t bufferCount) {
    if (socket == INVALID_SOCKET) {
        return std::make_tuple(0, SocketError::NotInitialized);
    }

    std::tuple<uint32_t, SocketError> res(0, SocketError::Ok);
    read_co(buffers, bufferCount, &res).get();
    return res;
}

std::future<void> SocketImpl::write_co(const INet::IoBuffer* buffers, uint32_t bufferCount, SocketError* res) {
    (*res) = co_await WriteAwaiter(this, buffers, bufferCount);
}

SocketError SocketImpl::write(const char* buffer, uint32_t bufferLen) {
    INet::IoBuffer ioBuffer;
    ioBuffer.buffer = (char*)buffer;
    ioBuffer.bufferLen = bufferLen;
    return writev(&ioBuffer, 1);
}

SocketError SocketImpl::writev(const INet::IoBuffer* buffers, uint32_t bufferCount) {
    if (socket == INVALID_SOCKET) {
        return SocketError::NotInitialized;
    }

    SocketError res = SocketError::Ok;
    write_co(buffers, bufferCount, &res).get();
    return res;
}

SocketError SocketImpl::shutdownRead() {
    if (socket == INVALID_SOCKET) {
        return SocketError::NotInitialized;
    }

    int res = ::shutdown(socket, SD_RECEIVE);
    if (res != 0) {
        int wsaErr = ::WSAGetLastError();
        return getSocketError(wsaErr);
    }

    return SocketError::Ok;
}

SocketError SocketImpl::shutdownWrite() {
    if (socket == INVALID_SOCKET) {
        return SocketError::NotInitialized;
    }

    int res = ::shutdown(socket, SD_SEND);
    if (res != 0) {
        int wsaErr = ::WSAGetLastError();
        return getSocketError(wsaErr);
    }

    return SocketError::Ok;
}

SocketError SocketImpl::setReadBuf(uint32_t bufferSize) {
    if (socket == INVALID_SOCKET) {
        return SocketError::NotInitialized;
    }

    if (bufferSize > INT_MAX) {
        bufferSize = INT_MAX;
    }

    int bufferOpt = (int)bufferSize;
    int setOptRes = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char*)&bufferOpt, sizeof(int));
    if (setOptRes != 0) {
        int wsaErr = ::WSAGetLastError();
        return getSocketError(wsaErr);
    }

    return SocketError::Ok;
}

SocketError SocketImpl::setWriteBuf(uint32_t bufferSize) {
    if (socket == INVALID_SOCKET) {
        return SocketError::NotInitialized;
    }

    if (bufferSize > INT_MAX) {
        bufferSize = INT_MAX;
    }

    int bufferOpt = (int)bufferSize;
    int setOptRes = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char*)&bufferOpt, sizeof(int));
    if (setOptRes != 0) {
        int wsaErr = ::WSAGetLastError();
        return getSocketError(wsaErr);
    }

    return SocketError::Ok;
}

#endif // _WIN32
