// socket_win.cpp

#ifndef _WIN32
#error "This file should only be included in a Windows build"
#endif

#include "cupcake/net/Socket.h"

#include "cupcake_priv/Cupcake_priv_win.h"
#include "cupcake_priv/SocketImpl_win.h"

#include <Ws2tcpip.h>
#include <Mstcpip.h>

#include <experimental/resumable>


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

// General TODO: Need to re-submit if WSAECONNRESET and the like
class SocketImpl::AcceptAwaiter {
public:
    AcceptAwaiter(SocketImpl* socketImpl) :
        socketImpl(socketImpl),
        overlappedData(),
        newSocket(nullptr),
        socketError(SocketError::Ok) {}

    bool await_ready() const {
        return false;
    }

    bool await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        overlappedData.coroutineHandle = coroutineHandle.to_address();
        overlappedData.completionResult = &completionResult;

        // TODO: Move before wait along with PTPIO bit
        // Initialize the socket
        SocketError err = initSocket(&acceptedSocket, socketImpl->localAddr.getFamily());
        if (err != SocketError::Ok) {
            socketError = err;
            return false;
        }

        ptpIo = ::CreateThreadpoolIo((HANDLE)acceptedSocket,
            completionCallback,
            this,
            NULL);

        if (ptpIo == NULL) {
            //DWORD err = ::GetLastError();
            socketError = SocketError::Unknown;
            return false;
        }

        // Trigger AcceptEx
        ::StartThreadpoolIo(socketImpl->ptpIo);

        BOOL res = PrivWin::getAcceptEx()(socketImpl->socket,
            acceptedSocket,
            addrBuffer,
            0,
            sizeof(SOCKET_ADDRESS) + 16,
            sizeof(SOCKET_ADDRESS) + 16,
            NULL,
            (OVERLAPPED*)&overlappedData);

        if (res) {
            ::CancelThreadpoolIo(socketImpl->ptpIo);
            onAccept();
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

    Result<Socket, SocketError> await_resume() {
        if (completionResult.error != ERROR_SUCCESS) {
            return Result<Socket, SocketError>(getSocketError((int)completionResult.error));
        } else {
            onAccept();
            return Result<Socket, SocketError>(Socket(newSocket));
        }
    }

private:
    /* Helper that does the work needed after an accept succeeds */
    void onAccept() {
        // Have to set the SO_UPDATE_ACCEPT_CONTEXT (see AcceptEx doc)
        int setSockRes = ::setsockopt(acceptedSocket,
            SOL_SOCKET,
            SO_UPDATE_ACCEPT_CONTEXT,
            (char *)&acceptedSocket,
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

        PrivWin::getGetAcceptExSockaddrs()(addrBuffer,
            0,
            sizeof(SOCKADDR_STORAGE) + 16,
            sizeof(SOCKADDR_STORAGE) + 16,
            &localPtr,
            &localLen,
            &remotePtr,
            &remoteLen);

        // Make a happy new Socket object
        newSocket = new SocketImpl();
        newSocket->socket = acceptedSocket;
        newSocket->ptpIo = ptpIo;
        // TODO: Use length values
        newSocket->localAddr = SockAddr::fromNative((const SOCKADDR_STORAGE*)localPtr);
        newSocket->remoteAddr = SockAddr::fromNative((const SOCKADDR_STORAGE*)remotePtr);
    }

    SocketImpl* socketImpl;
    OverlappedData overlappedData;
    CompletionResult completionResult;

    // Extra storage for AcceptEx
    SOCKET acceptedSocket;
    PTP_IO ptpIo;
    char addrBuffer[2 * (sizeof(SOCKADDR_STORAGE) + 16)];

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

        SOCKADDR_STORAGE storage;
        sockAddr.toNative(&storage);

        ::StartThreadpoolIo(socketImpl->ptpIo);

        BOOL res = PrivWin::getConnectEx()(socketImpl->socket,
            (const sockaddr*)&storage,
            sizeof(SOCKADDR_STORAGE),
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
        if (completionResult.error != ERROR_SUCCESS) {
            return getSocketError((int)completionResult.error);
        } else {
            onConnect();
            return SocketError::Ok;
        }
    }

private:
    /* Helper that does the work needed after a connect succeeds */
    void onConnect() {
        SOCKADDR_STORAGE storage;
        int nameLen = sizeof(SOCKADDR_STORAGE);
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
                char* buffer,
                uint32_t bufferLen) :
        socketImpl(socketImpl),
        overlappedData(),
        socketError(SocketError::Ok),
        bytesXfer(0) {
    
        buffers[0].buf = buffer;
        buffers[0].len = bufferLen;
    }

    bool await_ready() const {
        return false;
    }

    bool await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        overlappedData.coroutineHandle = coroutineHandle.to_address();
        overlappedData.completionResult = &completionResult;

        DWORD flags = 0;
        DWORD bytesRecv = 0;

        ::StartThreadpoolIo(socketImpl->ptpIo);

        int res = ::WSARecv(socketImpl->socket,
            buffers,
            1,
            &bytesRecv,
            &flags,
            (OVERLAPPED*)&overlappedData,
            NULL);

        if (res == 0) {
            ::CancelThreadpoolIo(socketImpl->ptpIo);
            bytesXfer = bytesRecv;
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

    Result<uint32_t, SocketError> await_resume() {
        if (socketError != SocketError::Ok) {
            return Result<uint32_t, SocketError>(socketError);
        } else if (bytesXfer) {
            return Result<uint32_t, SocketError>(bytesXfer);
        } else if (completionResult.error != ERROR_SUCCESS) {
            return Result<uint32_t, SocketError>(getSocketError((int)completionResult.error));
        } else {
            return Result<uint32_t, SocketError>((uint32_t)completionResult.bytesTransfered);
        }
    }

private:
    SocketImpl* socketImpl;
    WSABUF buffers[1];
    OverlappedData overlappedData;
    CompletionResult completionResult;

    // Result value
    SocketError socketError;
    uint32_t bytesXfer;
};

class SocketImpl::WriteAwaiter {
public:
    WriteAwaiter(SocketImpl* socketImpl,
                 const char* buffer,
                 uint32_t bufferLen) :
        socketImpl(socketImpl),
        overlappedData(),
        socketError(SocketError::Ok),
        bytesXfer(0) {

        buffers[0].buf = (char*)buffer;
        buffers[0].len = bufferLen;
    }

    bool await_ready() const {
        return false;
    }

    bool await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        overlappedData.coroutineHandle = coroutineHandle.to_address();
        overlappedData.completionResult = &completionResult;

        DWORD bytesSent;

        ::StartThreadpoolIo(socketImpl->ptpIo);

        int res = ::WSASend(socketImpl->socket,
            buffers,
            1,
            &bytesSent,
            0,
            (OVERLAPPED*)&overlappedData,
            NULL);

        if (res == 0) {
            ::CancelThreadpoolIo(socketImpl->ptpIo);
            bytesXfer = bytesSent;
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

    Result<uint32_t, SocketError> await_resume() {
        if (socketError != SocketError::Ok) {
            return Result<uint32_t, SocketError>(socketError);
        } else if (bytesXfer) {
            return Result<uint32_t, SocketError>(bytesXfer);
        } else if (completionResult.error != ERROR_SUCCESS) {
            return Result<uint32_t, SocketError>(getSocketError((int)completionResult.error));
        } else {
            return Result<uint32_t, SocketError>((uint32_t)completionResult.bytesTransfered);
        }
    }

private:
    SocketImpl* socketImpl;
    WSABUF buffers[1];
    OverlappedData overlappedData;
    CompletionResult completionResult;

    // Result value
    SocketError socketError;
    uint32_t bytesXfer;
};

SocketImpl::OverlappedData::OverlappedData() :
	coroutineHandle(nullptr),
    completionResult(nullptr) {
	::memset(&overlapped, 0, sizeof(OVERLAPPED));
}

SocketImpl::SocketImpl() :
    socket(INVALID_SOCKET),
    ptpIo(NULL),
    localAddr(),
    remoteAddr()
{}

SocketImpl::~SocketImpl() {
    close();
}

void SocketImpl::close() {
    if (socket != INVALID_SOCKET) {
        ::closesocket(socket);
        socket = INVALID_SOCKET;
    }
}

SockAddr SocketImpl::getLocalAddress() const {
    return localAddr;
}

SockAddr SocketImpl::getRemoteAddress() const {
    return remoteAddr;
}

SocketError SocketImpl::bind(const SockAddr& sockAddr) {
    if (socket != INVALID_SOCKET) {
        return SocketError::InvalidState;
    }

    SOCKADDR_STORAGE storage;
    sockAddr.toNative(&storage);

    SocketError err = initSocket(storage.ss_family);
    if (err != SocketError::Ok) {
        return err;
    }

    // For bind we also want to set the SO_EXCLUSIVEADDRUSE option
    DWORD opt = 1;
    int optRes = ::setsockopt(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&opt, sizeof(DWORD));
    if (optRes != 0) {
        return getSocketError(::WSAGetLastError());
    }

    int bindRes = ::bind(socket, (const sockaddr*)&storage, sizeof(SOCKADDR_STORAGE));
    if (bindRes != 0) {
        return getSocketError(::WSAGetLastError());
    }

	// And then we need to fetch back the bound addr/port
	int nameLen = sizeof(SOCKADDR_STORAGE);
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
    if (queue > SOMAXCONN) {
        queue = SOMAXCONN;
    }

    int res = ::listen(socket, (int)queue);
    if (res != 0) {
        return getSocketError(::WSAGetLastError());
    }

    return SocketError::Ok;
}

std::future<void> SocketImpl::accept_co(Result<Socket, SocketError>* res) {
    (*res) = await AcceptAwaiter(this);
}

Result<Socket, SocketError> SocketImpl::accept() {
    if (socket == INVALID_SOCKET) {
        return Result<Socket, SocketError>(SocketError::InvalidState);
    }

    Result<Socket, SocketError> res(SocketError::Ok);
    accept_co(&res).get();
    return std::move(res);
}

std::future<void> SocketImpl::connect_co(const SockAddr& sockAddr, SocketError* res) {
    (*res) = await ConnectAwaiter(this, sockAddr);
}

SocketError SocketImpl::connect(const SockAddr& sockAddr) {
    if (socket != INVALID_SOCKET) {
        return SocketError::InvalidState;
    }

    SOCKADDR_STORAGE storage;
    sockAddr.toNative(&storage);

    // Initialize our socket
    SocketError err = initSocket(storage.ss_family);
    if (err != SocketError::Ok) {
        return err;
    }

	// For reasons that I presume have to do with DisconnectEx, ConnectEx
	// requires a bound socket
	SOCKADDR_STORAGE addrAny;
	addrAny.ss_family = storage.ss_family;
	INETADDR_SETANY((sockaddr*)&addrAny);

	int bindRes = ::bind(socket, (const sockaddr*)&addrAny, sizeof(SOCKADDR_STORAGE));
	if (bindRes != 0) {
		return getSocketError(::WSAGetLastError());
	}

    SocketError res;
    connect_co(sockAddr, &res).get();
    return res;
}

std::future<void> SocketImpl::read_co(char* buffer, uint32_t bufferLen, Result<uint32_t, SocketError>* res) {
    (*res) = await ReadAwaiter(this, buffer, bufferLen);
}

Result<uint32_t, SocketError> SocketImpl::read(char* buffer, uint32_t bufferLen) {
    if (socket == INVALID_SOCKET) {
        return Result<uint32_t, SocketError>(SocketError::InvalidState);
    }

    Result<uint32_t, SocketError> res(SocketError::Ok);
    read_co(buffer, bufferLen, &res).get();
    return res;
}

std::future<void> SocketImpl::write_co(const char* buffer, uint32_t bufferLen, Result<uint32_t, SocketError>* res) {
    (*res) = await WriteAwaiter(this, buffer, bufferLen);
}

Result<uint32_t, SocketError> SocketImpl::write(const char* buffer, uint32_t bufferLen) {
    if (socket == INVALID_SOCKET) {
        return Result<uint32_t, SocketError>(SocketError::InvalidState);
    }

    Result<uint32_t, SocketError> res(SocketError::Ok);
    write_co(buffer, bufferLen, &res).get();
    return res;
}

SocketError SocketImpl::initSocket(int family) {
    SocketError err = initSocket(&socket, family);
    if (err != SocketError::Ok) {
        return err;
    }

    // Create a thread pool IO for the handle
    ptpIo = ::CreateThreadpoolIo((HANDLE)socket,
        completionCallback,
        this,
        NULL);
    if (ptpIo == NULL) {
        //DWORD err = ::GetLastError();
        return SocketError::Unknown;
    }

    return SocketError::Ok;
}

SocketError SocketImpl::initSocket(SOCKET* pSocket, int family) {
    (*pSocket) = INVALID_SOCKET;

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
            return getSetsockoptError(wsaErr);
        }
    }
	*/

    (*pSocket) = socket;
    return SocketError::Ok;
}

void CALLBACK SocketImpl::completionCallback(PTP_CALLBACK_INSTANCE instance,
        PVOID context,
        PVOID overlapped,
        ULONG ioResult,
        ULONG_PTR numberOfBytesTransferred,
        PTP_IO ptpIo) {

    OverlappedData* data = (OverlappedData*)overlapped;
    data->completionResult->error = ioResult;
    data->completionResult->bytesTransfered = numberOfBytesTransferred;
    
    std::experimental::coroutine_handle<> coroutineHandle =
        std::experimental::coroutine_handle<>::from_address(data->coroutineHandle);
    coroutineHandle.resume();
}
