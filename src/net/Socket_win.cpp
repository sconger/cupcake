// socket_win.cpp

#ifndef _WIN32
#error "This file should only be included in a Windows build"
#endif

#include "cupcake/net/Socket.h"

#include "cupcake_priv/Cupcake_priv_win.h"

#include <Ws2tcpip.h>
#include <Mstcpip.h>

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

Socket::overlappedData::overlappedData(Socket* asyncSocket) :
	context(nullptr),
	socket(asyncSocket),
	acceptCallback(nullptr),
	connectCallback(nullptr),
	xferCallback(nullptr) {
	::memset(&overlapped, 0, sizeof(OVERLAPPED));
}

void Socket::overlappedData::reset() {
	::memset(&overlapped, 0, sizeof(OVERLAPPED));
	context = nullptr;
	socket = nullptr;
	acceptCallback = nullptr;
	connectCallback = nullptr;
	xferCallback = nullptr;
}

Socket::Socket() noexcept :
    socket(INVALID_SOCKET),
    ptpIo(NULL),
	readOverlapped(this),
	writeOverlapped(this),
    localAddr(),
    remoteAddr()
{}

Socket::~Socket() {
    close();
}

void Socket::close() {
    if (socket != INVALID_SOCKET) {
        ::closesocket(socket);
        socket = INVALID_SOCKET;
    }
}

SockAddr Socket::getLocalAddress() const {
    return localAddr;
}

SockAddr Socket::getRemoteAddress() const {
    return remoteAddr;
}

SocketError Socket::bind(const SockAddr& sockAddr) {
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

SocketError Socket::listen() {
    return listen(SOMAXCONN);
}

SocketError Socket::listen(uint32_t queue) {
    if (queue > SOMAXCONN) {
        queue = SOMAXCONN;
    }

    int res = ::listen(socket, (int)queue);
    if (res != 0) {
        return getSocketError(::WSAGetLastError());
    }

    return SocketError::Ok;
}

SocketError Socket::accept(void* context, const AcceptCallback& callback) {
    if (socket == INVALID_SOCKET) {
        return SocketError::InvalidState;
    }

    // Initialize the overlapped data, including creating a socket
    ::memset(&readOverlapped, 0, sizeof(overlappedData));
    readOverlapped.ioType = IoType::Accept;
    readOverlapped.context = context;
    readOverlapped.socket = this;
	readOverlapped.acceptCallback = callback;

    SocketError err = initSocket(&readOverlapped.acceptData.acceptedSocket, localAddr.getFamily());
    if (err != SocketError::Ok) {
        return err;
    }

    ::StartThreadpoolIo(ptpIo);

    BOOL res = PrivWin::getAcceptEx()(socket,
        readOverlapped.acceptData.acceptedSocket,
        readOverlapped.acceptData.addrBuffer,
        0,
        sizeof(SOCKET_ADDRESS) + 16,
        sizeof(SOCKET_ADDRESS) + 16,
        NULL,
        (OVERLAPPED*)&readOverlapped);

    if (res) {
        ::CancelThreadpoolIo(ptpIo);
        onAccept(&readOverlapped);
    } else {
        int wsaErr = ::WSAGetLastError();

        if (wsaErr != WSA_IO_PENDING) {
            ::CancelThreadpoolIo(ptpIo);
            return getSocketError(wsaErr);
        }
    }

    return SocketError::Ok;
}

SocketError Socket::connect(void* context, const SockAddr& sockAddr, const ConnectCallback& callback) {
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

    // Initialize the overlapped data, including creating a socket
    ::memset(&writeOverlapped, 0, sizeof(overlappedData));
    writeOverlapped.ioType = IoType::Connect;
    writeOverlapped.context = context;
    writeOverlapped.socket = this;
    writeOverlapped.connectCallback = callback;

    ::StartThreadpoolIo(ptpIo);

    BOOL res = PrivWin::getConnectEx()(socket,
        (const sockaddr*)&storage,
        sizeof(SOCKADDR_STORAGE),
        NULL,
        0,
        NULL,
        (OVERLAPPED*)&writeOverlapped);

    if (res) {
        ::CancelThreadpoolIo(ptpIo);
        onConnect(&writeOverlapped);
    } else {
        int wsaErr = ::WSAGetLastError();

        if (wsaErr != WSA_IO_PENDING) {
            ::CancelThreadpoolIo(ptpIo);
            return getSocketError(wsaErr);
        }
    }

    return SocketError::Ok;
}

SocketError Socket::read(void* context, char* buffer, uint32_t bufferLen, const XferCallback& callback) {
    if (socket == INVALID_SOCKET) {
        return SocketError::InvalidState;
    }

    // Initialize the overlapped data, including creating a socket
    ::memset(&readOverlapped, 0, sizeof(overlappedData));
    readOverlapped.ioType = IoType::Read;
    readOverlapped.context = context;
    readOverlapped.socket = this;
    readOverlapped.xferCallback = callback;

    // Set up a single WSABUF
    WSABUF buffers[1];
    buffers[0].buf = (char*)buffer;
    buffers[0].len = bufferLen;

    DWORD bytesRecv;

    ::StartThreadpoolIo(ptpIo);

    int res = ::WSARecv(socket,
        buffers,
        1,
        NULL,
        &bytesRecv,
        (OVERLAPPED*)&readOverlapped,
        NULL);

    if (res == 0) {
        ::CancelThreadpoolIo(ptpIo);
        onRead(&writeOverlapped, bytesRecv);
    } else {
        int wsaErr = ::WSAGetLastError();

        if (wsaErr != WSA_IO_PENDING) {
            ::CancelThreadpoolIo(ptpIo);
            return getSocketError(wsaErr);
        }
    }

    return SocketError::Ok;
}

SocketError Socket::write(void* context, const char* buffer, uint32_t bufferLen, const XferCallback& callback) {
    if (socket == INVALID_SOCKET) {
        return SocketError::InvalidState;
    }

    // Initialize the overlapped data, including creating a socket
    ::memset(&writeOverlapped, 0, sizeof(overlappedData));
    writeOverlapped.ioType = IoType::Write;
    writeOverlapped.context = context;
    writeOverlapped.socket = this;
    writeOverlapped.xferCallback = callback;

    // Set up a single WSABUF
    WSABUF buffers[1];
    buffers[0].buf = (char*)buffer;
    buffers[0].len = bufferLen;

    DWORD bytesSent;

    ::StartThreadpoolIo(ptpIo);

    int res = ::WSASend(socket,
        buffers,
        1,
        &bytesSent,
        0,
        (OVERLAPPED*)&writeOverlapped,
        NULL);

    if (res == 0) {
        ::CancelThreadpoolIo(ptpIo);
        onWrite(&writeOverlapped, bytesSent);
    } else {
        int wsaErr = ::WSAGetLastError();

        if (wsaErr != WSA_IO_PENDING) {
            ::CancelThreadpoolIo(ptpIo);
            return getSocketError(wsaErr);
        }
    }

    return SocketError::Ok;
}

SocketError Socket::initSocket(int family) {
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

SocketError Socket::initSocket(SOCKET* pSocket, int family) {
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

void Socket::onAccept(overlappedData* data) {
    // Have to set the SO_UPDATE_ACCEPT_CONTEXT (see AcceptEx doc)
    int setSockRes = ::setsockopt(data->acceptData.acceptedSocket,
        SOL_SOCKET,
        SO_UPDATE_ACCEPT_CONTEXT,
        (char *)&data->socket->socket,
        sizeof(SOCKET));

    if (setSockRes != 0) {
        int wsaErr = ::WSAGetLastError();
        SocketError err = getSocketError(wsaErr);

        // Trigger the callback
        data->acceptCallback(data->context, nullptr, err);
		data->acceptCallback = nullptr;
        return;
    }

    // Parse out the address information
    SOCKADDR* localPtr;
    INT localLen;
    SOCKADDR* remotePtr;
    INT remoteLen;

    PrivWin::getGetAcceptExSockaddrs()(data->acceptData.addrBuffer,
        0,
        sizeof(SOCKADDR_STORAGE) + 16,
        sizeof(SOCKADDR_STORAGE) + 16,
        &localPtr,
        &localLen,
        &remotePtr,
        &remoteLen);

    // Make a happy new Socket object
    Socket* newSock = new Socket();
    newSock->socket = data->acceptData.acceptedSocket;
    ::memcpy(&newSock->localAddr, localPtr, localLen);
    ::memcpy(&newSock->remoteAddr, remotePtr, remoteLen);

    // Trigger the callback
    data->acceptCallback(data->context, newSock, SocketError::Ok);
}

void Socket::onConnect(overlappedData* data) {
	SOCKADDR_STORAGE storage;
	int nameLen = sizeof(SOCKADDR_STORAGE);
	int nameRes = ::getsockname(data->socket->socket, (sockaddr*)&storage, &nameLen);
	if (nameRes != 0) {
        SocketError err = getSocketError(::WSAGetLastError());
		data->connectCallback(data->context, err);
		return;
	}

    // Copy in the remote address
    data->socket->remoteAddr.fromNative(&storage);

    // Trigger the callback
    data->connectCallback(data->context, SocketError::Ok);
}

void Socket::onRead(overlappedData* data, uint64_t bytesXfer) {
    data->xferCallback(data->context, bytesXfer, SocketError::Ok);
}

void Socket::onWrite(overlappedData* data, uint64_t bytesXfer) {
    data->xferCallback(data->context, bytesXfer, SocketError::Ok);
}

void CALLBACK Socket::completionCallback(PTP_CALLBACK_INSTANCE instance,
        PVOID context,
        PVOID overlapped,
        ULONG ioResult,
        ULONG_PTR numberOfBytesTransferred,
        PTP_IO ptpIo) {

    overlappedData* data = (overlappedData*)overlapped;

    if (ioResult == ERROR_SUCCESS) {
        switch (data->ioType) {
        case IoType::Accept:
            onAccept(data);
            break;
        case IoType::Connect:
            onConnect(data);
            break;
        case IoType::Read:
            onRead(data, numberOfBytesTransferred);
            break;
        case IoType::Write:
            onWrite(data, numberOfBytesTransferred);
            break;
        }
    } else {
        SocketError err;

        switch (data->ioType) {
        case IoType::Accept:
            err = getSocketError((int)ioResult);
            data->acceptCallback(data->context, nullptr, err);
            break;
        case IoType::Connect:
            err = getSocketError((int)ioResult);
            data->connectCallback(data->context, err);
            break;
        case IoType::Read:
            err = getSocketError((int)ioResult);
            data->xferCallback(data->context, 0, err);
            break;
        case IoType::Write:
            err = getSocketError((int)ioResult);
            data->xferCallback(data->context, 0, err);
            break;
        }
    }
}
