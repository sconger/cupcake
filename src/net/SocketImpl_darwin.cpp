
#ifdef __APPLE__

// Note that this file contains some objective-C

// General TODO: Should create dispatch_source_t objects once and just pause them

#include "cupcake/net/Socket.h"

#include "cupcake_priv/net/SocketImpl_darwin.h"

#include <experimental/resumable>

#include <errno.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace Cupcake;


static SocketError getSocketError(int errVal) {
    switch (errVal) {
        case EBADF:
        case ENOTSOCK:
            return SocketError::InvalidHandle;
        case EMFILE:
        case ENFILE:
            return SocketError::TooManyHandles;
        case EACCES:
        case EROFS:
            return SocketError::AccessDenied;
        case ENOBUFS:
            return SocketError::NoBufferSpace;
        case ENOMEM:
            return SocketError::OutOfMemory;
        case EDESTADDRREQ:
            return SocketError::NotBound;
        case EADDRINUSE:
            return SocketError::AddressInUse;
        case EADDRNOTAVAIL:
            return SocketError::AddressNotAvailable;
        case EAFNOSUPPORT:
        case EOPNOTSUPP:
        case EPROTONOSUPPORT:
        case EPROTOTYPE:
            return SocketError::NotSupported;
        case ECONNREFUSED:
            return SocketError::ConnectionRefused;
        case ECONNABORTED:
            return SocketError::ConnectionAborted;
        case EFAULT:
            return SocketError::BadAddress;
        case EHOSTUNREACH:
            return SocketError::HostUnreachable;
        case EINVAL:
        case EDOM:
            return SocketError::InvalidArgument;
        case EISCONN:
            return SocketError::AlreadyConnected;
        case ENETDOWN:
            return SocketError::NetworkDown;
        case ENETUNREACH:
            return SocketError::NetworkUnreachable;
        case ETIMEDOUT:
            return SocketError::TimedOut;
        case ECONNRESET:
            return SocketError::ConnectionReset;
        case EIO:
            return SocketError::IoError;
        default:
            return SocketError::Unknown;
    }
}

// OSX is picky about telling it the exact address length
static
int storageLen(const sockaddr_storage* storage) {
    if (storage->ss_family == AF_INET) {
        return sizeof(sockaddr_in);
    } else {
        return sizeof(sockaddr_in6);
    }
}

SocketError SocketImpl::setSocket(int newSocket, int family) {
    // Mark the socket as close on exec
    int fcntlErr = ::fcntl(newSocket, F_SETFD, FD_CLOEXEC);
    if (fcntlErr != -1) {
        // TODO: Log
        return SocketError::Unknown;
    }
    
    // TODO: For IPv6 sockets, disable IPv6 only mode
    
    // Turn off Nagle's algorithm
    int one = 1;
    int noDelayErr = ::setsockopt(newSocket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    if (noDelayErr == -1) {
        // TODO: log
        return SocketError::Unknown;
    }
    
    // Gcd documentation suggests NOSIGPIPE is required for accepts
    int noPipeFlag = 1;
    int noSigPipeRes = ::setsockopt(newSocket, SOL_SOCKET, SO_NOSIGPIPE, &noPipeFlag, sizeof(int));
    if (noSigPipeRes == -1) {
        // TODO: log
        return SocketError::Unknown;
    }

    long getFlRes = ::fcntl(fd, F_GETFL);
    if (getFlRes < 0) {
        // TODO: log
        return SocketError::Unknown;
    }

    long setFlRes = ::fcntl(fd, F_SETFL, getFlRes | O_NONBLOCK);
    if (setFlRes == -1) {
        // TODO: log
        return SocketError::Unknown;
    }
    /*
    // Note that gcd expects O_NONBLOCK, but it's set automatically by
    // dispatch_io_create

    dispatch_queue_t dispatchQueue = ::dispatch_get_global_queue(QOS_CLASS_UTILITY, 0);
    dispatchIo = dispatch_io_create(DISPATCH_IO_STREAM, newSocket, dispatchQueue, ^(int error) {});
    if (dispatchIo == NULL) {
        // TODO: log
        return SocketError::Unknown;
    }
     */

    fd = newSocket;
    return SocketError::Ok;
}

class SocketImpl::AcceptAwaiter {
public:
    AcceptAwaiter(SocketImpl* socketImpl) :
        socketImpl(socketImpl),
        newSocket(nullptr),
        socketError(SocketError::Ok)
    {}
    
    bool await_ready() {
        std::tie(newSocket, socketError) = socketImpl->tryAccept();

        if (newSocket || socketError != SocketError::Ok) {
            return true;
        }

        ::dispatch_set_context(socketImpl->readSource, (void*)this);
        ::dispatch_source_set_event_handler_f(socketImpl->readSource, dispatchCallback);
        ::dispatch_resume(socketImpl->readSource);

        return false;
    }
    
    void await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        this->coroutineHandle = coroutineHandle;
    }
    
    std::tuple<Socket, SocketError> await_resume() {
        if (socketError != SocketError::Ok) {
            return std::make_tuple(Socket(), socketError);
        }
        return std::make_tuple(Socket(newSocket), SocketError::Ok);
    }
    
private:
    static
    void dispatchCallback(void* voidAwaiter) {
        AcceptAwaiter* awaiter = (AcceptAwaiter*)voidAwaiter;

        // Try once more to accept
        std::tie(awaiter->newSocket, awaiter->socketError) = awaiter->socketImpl->tryAccept();

        // If it succeeded or failed, clear the dispatch source and resume
        if (awaiter->newSocket || awaiter->socketError != SocketError::Ok) {
            ::dispatch_source_cancel(awaiter->socketImpl->readSource);
            awaiter->coroutineHandle.resume();
            return;
        }

        // Otherwise, let the callback trigger again
    }
    
    SocketImpl* socketImpl;
    std::experimental::coroutine_handle<> coroutineHandle;
    
    // Result values
    SocketImpl* newSocket;
    SocketError socketError;
};

class SocketImpl::ConnectAwaiter {
public:
    ConnectAwaiter(SocketImpl* socketImpl, const sockaddr_storage& sockAddr) :
        socketImpl(socketImpl),
        sockAddr(sockAddr),
        connected(false),
        socketError(SocketError::Ok)
    {}
    
    bool await_ready() {
        std::tie(connected, socketError) = socketImpl->tryConnect(sockAddr);
        
        if (connected || socketError != SocketError::Ok) {
            return true;
        }

        ::dispatch_set_context(socketImpl->writeSource, (void*)this);
        ::dispatch_source_set_event_handler_f(socketImpl->writeSource, dispatchCallback);
        ::dispatch_resume(socketImpl->writeSource); // Starts suspended
        
        return false;
    }
    
    void await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        this->coroutineHandle = coroutineHandle;
    }
    
    SocketError await_resume() {
        return socketError;
    }
    
private:
    static
    void dispatchCallback(void* voidAwaiter) {
        ConnectAwaiter* awaiter = (ConnectAwaiter*)voidAwaiter;
        
        // Try once more to connect
        std::tie(awaiter->connected, awaiter->socketError) = awaiter->socketImpl->tryConnect(sockAddr);
        
        // If it succeeded or failed, clear the dispatch source and resume
        if (awaiter->connected || awaiter->socketError != SocketError::Ok) {
            ::dispatch_source_cancel(awaiter->socketImpl->writeSource);
            awaiter->coroutineHandle.resume();
            return;
        }
        
        // Otherwise, let the callback trigger again
    }
    
    SocketImpl* socketImpl;
    const sockaddr_storage& sockAddr;
    std::experimental::coroutine_handle<> coroutineHandle;
    
    // Result values
    bool connected;
    SocketError socketError;
};

class SocketImpl::ReadAwaiter {
public:
    ReadAwaiter(SocketImpl* socketImpl,
                iovec* iov,
                int iovcnt) :
        socketImpl(socketImpl),
        iov(iov),
        iovcnt(iovcnt),
        bytesRead(0),
        socketError(SocketError::Ok)
    {}
    
    bool await_ready() {
        std::tie(bytesRead, socketError) = socketImpl->tryRead(awaiter->iov, awaiter->iovcnt);
        
        if (bytesRead >= 0 || socketError != SocketError::Ok) {
            return true;
        }

        ::dispatch_set_context(socketImpl->readSource, (void*)this);
        ::dispatch_source_set_event_handler_f(socketImpl->readSource, dispatchCallback);
        ::dispatch_resume(socketImpl->readSource);
        
        return false;
    }
    
    void await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        this->coroutineHandle = coroutineHandle;
    }
    
    std::tuple<uint32_t, SocketError> await_resume() {
        return std::make_tuple(bytesRead, socketError);
    }
    
private:
    static
    void dispatchCallback(void* voidAwaiter) {
        ReadAwaiter* awaiter = (ReadAwaiter*)voidAwaiter;
        
        // Try once more to read
        std::tie(awaiter->bytesRead, awaiter->socketError) =
                awaiter->socketImpl->tryRead(awaiter->iov, awaiter->iovcnt);
        
        // If it succeeded or failed, clear the dispatch source and resume
        if (awaiter->bytesRead >= 0 || awaiter->socketError != SocketError::Ok) {
            ::dispatch_source_cancel(awaiter->socketImpl->readSource);
            awaiter->coroutineHandle.resume();
            return;
        }
        
        // Otherwise, let the callback trigger again
    }
    
    SocketImpl* socketImpl;
    iovec* iov;
    int iovcnt;
    std::experimental::coroutine_handle<> coroutineHandle;
    
    // Result values
    ssize_t bytesRead;
    SocketError socketError;
};

// Note that this mutates the passed IoBuffers
class SocketImpl::WriteAwaiter {
public:
    WriteAwaiter(SocketImpl* socketImpl,
                 iovec* iov,
                 int iovcnt) :
        socketImpl(socketImpl),
        iov(iov),
        iovcnt(iovcnt),
        dataWritten(0),
        socketError(SocketError::Ok) {

        totalData = 0;
        for (size_t i = 0; i < bufferCount; i++) {
            totalData += buffers[i].bufferLen;
        }
    }
    
    bool await_ready() {
        ssize_t bytesWritten;
        std::tie(bytesWritten, socketError) = socketImpl->tryWrite(iov, iovcnt);
        
        if (bytesWritten == totalData || socketError != SocketError::Ok) {
            return true;
        }

        dataWritten += bytesWritten;
        
        ::dispatch_set_context(socketImpl->writeSource, (void*)this);
        ::dispatch_source_set_event_handler_f(socketImpl->writeSource, dispatchCallback);
        ::dispatch_resume(socketImpl->writeSource);
        
        return false;
    }
    
    void await_suspend(std::experimental::coroutine_handle<> coroutineHandle) {
        this->coroutineHandle = coroutineHandle;
    }
    
    SocketError await_resume() {
        return socketError;
    }
    
private:
    static
    void dispatchCallback(void* voidAwaiter) {
        WriteAwaiter* awaiter = (WriteAwaiter*)voidAwaiter;

        // We need to adjust the buffers as we may have partially written data
        while (dataWritten > 0) {
            size_t removable = std::min(iov->iov_len, dataWritten);
            iov->iov_len -= removable;
            iov->ov_base += removable;
            totalData -= removable;
            dataWritten -= removable;
            if (iov->iov_len == 0) {
                iov++;
                iovcnt--;
            }
        }
        
        // Try once more to write
        ssize_t bytesWritten;
        std::tie(bytesWritten, awaiter->socketError) =
            awaiter->socketImpl->tryWrite(awaiter->buffers, awaiter->bufferCount);

        // On failure, resume
        if (bytesWritten == -1) {
            ::dispatch_source_cancel(awaiter->socketImpl->writeSource);
            awaiter->coroutineHandle.resume();
            return;
        }

        awaiter->dataWritten += bytesWritten;
        
        // If we wrote all the bytes, resume
        if (awaiter->dataWritten == awaiter->totalData) {
            ::dispatch_source_cancel(awaiter->socketImpl->writeSource);
            awaiter->coroutineHandle.resume();
            return;
        }
        
        // Otherwise, let the callback trigger again
    }
    
    SocketImpl* socketImpl;
    iovec* iov;
    int iovcnt;
    size_t dataWritten;
    size_t totalData;
    std::experimental::coroutine_handle<> coroutineHandle;
    
    // Result values
    SocketError socketError;
};

std::tuple<SocketImpl*, SocketError> SocketImpl::tryAccept() {
    bool retry = true;
    sockaddr_storage storage;
    socklen_t addrLen;
    int newFd;
    
    do {
        newFd = ::accept(fd, (sockaddr*)&storage, &addrLen);
        if (newFd == -1) {
            int err = errno;
            // If the error is something where we should just retry, then retry
            if (err == ECONNABORTED) {
                continue;
            }

            // If we get an indication of blocking, return a null with no error indicated
            if (err == EWOULDBLOCK) {
                return std::make_tuple(nullptr, SocketError::Ok);
            }
            
            // Otherwise, return an error
            return std::make_tuple(nullptr, getSocketError(err));
        }
        retry = false;
    } while (retry);
    
    SocketImpl* newSocket = new SocketImpl();
    SocketError socketError = newSocket->setSocket(newFd, storage.ss_family);
    if (socketError != SocketError::Ok) {
        delete newSocket;
        return std::make_tuple(nullptr, socketError);
    }
    newSocket->localAddr = localAddr;
    newSocket->remoteAddr = SockAddr::fromNative(&storage);

    return std::make_tuple(newSocket, SocketError::Ok);
}

std::tuple<bool, SocketError> SocketImpl::tryConnect(const sockaddr_storage& storage) {
    socklen_t addrLen;
    
    if (storage.ss_family == AF_INET) {
        addrLen = sizeof(sockaddr_in);
    } else {
        addrLen = sizeof(sockaddr_in6);
    }

    int res = ::connect(fd, (const sockaddr*)&storage, addrLen);
    
    if (res == -1) {
        int err = errno;
        
        if (err == EINPROGRESS ||
            err == EALREADY) {
            return std::make_tuple(false, SocketError::Ok);
        } else {
            return std::make_tuple(false, getSocketError(err));
        }
    } else {
        // If the connect succeeded, grab the local and remote addrs
        sockaddr_storage localStorage, remoteStorage;
        socklen_t nameLen = sizeof(sockaddr_storage);
        int localErr = ::getsockname(fd, (sockaddr*)&localStorage, &nameLen);
        if (localErr != 0) {
            return std::make_tuple(false, getSocketError(errno));
        } else {
            int remoteErr = ::getpeername(fd, (sockaddr*)&remoteStorage, &nameLen);
            if (remoteErr != 0) {
                return std::make_tuple(false, getSocketError(errno));
            } else {
                localAddr = SockAddr::fromNative(&localStorage);
                remoteAddr = SockAddr::fromNative(&remoteStorage);
            }
        }
    }
    
    return std::make_tuple(true, SocketError::Ok);
}

std::tuple<ssize_t, SocketError> SocketImpl::tryRead(iovec *iov, int iovcnt) {
    ssize_t res = ::readv(fd, iov, iovcnt);
    if (res == -1) {
        int err == errno;
        
        if (err == EAGAIN) {
            return std::make_tuple(-1, SocketError::Ok);
        } else {
            return std::make_tuple(-1, getSocketError(err));
        }
    }
    
    return std::make_tuple(res, SocketError::Ok);
}

std::tuple<ssize_t, SocketError> SocketImpl::tryWrite(const iovec *iov, int iovcnt) {
    ssize_t res = ::writev(fd, iov, iovcnt);
    if (res == -1) {
        int err == errno;
        
        if (err == EAGAIN) {
            return std::make_tuple(-1, SocketError::Ok);
        } else {
            return std::make_tuple(-1, getSocketError(err));
        }
    }
    
    return std::make_tuple(res, SocketError::Ok);
}

SocketImpl::SocketImpl() :
    fd(-1),
    localAddr(),
    remoteAddr()
{}

SocketImpl::~SocketImpl() {
    close();
}

SocketError SocketImpl::init(INet::Protocol prot) {
    if (fd != -1) {
        return SocketError::InvalidState;
    }
    
    int family;
    
    if (prot == INet::Protocol::Ipv4) {
        family = AF_INET;
    } else if (prot == INet::Protocol::Ipv6) {
        family = AF_INET6;
    } else {
        return SocketError::InvalidArgument;
    }

    fd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        return getSocketError(errno);
    }

    SocketError err = setSocket(newSocket, family);
    if (err != SocketError::Ok) {
        ::close(fd);
    }

    dispatch_queue_t globalQueue = ::dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    readSource = ::dispatch_source_create(DISPATCH_SOURCE_TYPE_READ,
                                          fd, 0, globalQueue);
    if (readSouce == nullptr) {
        ::close(fd);
        return SocketError::Unknown;
    }
    writeSource = ::dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE,
                                           fd, 0, globalQueue);
    if (writeSource == nullptr) {
        ::close(fd);
        ::dispatch_release(readSource);
        return SocketError::Unknown;
    }
    readSource
    return err;
}

SocketError SocketImpl::close() {
    if (fd != -1) {
        ::dispatch_release(readSource);
        ::dispatch_release(writeSource);
        
        int closeRes = ::close(fd);
        fd = -1;
        if (closeRes == -1) {
            return getSocketError(errno);
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
    if (fd != -1) {
        return SocketError::InvalidState;
    }
    
    sockaddr_storage storage;
    sockAddr.toNative(&storage);
    
    // TODO: Check if address is of correct type
    
    // It is presumed that you'll want REUSEADDR for an accepting socket
    int reuseAddrFlag = 1;
    int reuseAddrRes = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseAddrFlag, sizeof(int));
    if (reuseAddrRes == -1) {
        return getSocketError(errno);
    }
    
    int reusePortFlag = 1;
    int reusePortRes = ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reusePortFlag, sizeof(int));
    if (reusePortRes == -1) {
        return getSocketError(errno);
    }
    
    // Bind
    int bindRes = ::bind(fd, (const sockaddr*)&storage, storageLen(&storage));
    if (bindRes == -1) {
        return getSocketError(errno);
    }
    
    // And then we need to fetch back the bound addr/port
    socklen_t nameLen = sizeof(sockaddr_storage);
    int nameErr = ::getsockname(fd, (sockaddr*)&storage, &nameLen);
    if (nameErr != 0) {
        return getSocketError(errno);
    }
    
    localAddr = SockAddr::fromNative(&storage);
    return SocketError::Ok;
}

SocketError SocketImpl::listen() {
    return listen(SOMAXCONN);
}

SocketError SocketImpl::listen(uint32_t queue) {
    if (fd == -1) {
        return SocketError::NotInitialized;
    }
    
    if (queue > SOMAXCONN) {
        queue = SOMAXCONN;
    }
    
    int res = ::listen(fd, (int)queue);
    if (res != 0) {
        return getSocketError(errno);
    }
    
    return SocketError::Ok;
}

std::future<void> SocketImpl::accept_co(std::tuple<Socket, SocketError>* res) {
    (*res) = co_await AcceptAwaiter(this);
}

std::tuple<Socket, SocketError> SocketImpl::accept() {
    if (fd == -1) {
        return std::make_tuple(Socket(), SocketError::NotInitialized);
    }
    
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
    accept_co(&res).get();
    return std::move(res);
}

std::future<void> SocketImpl::connect_co(const sockaddr_storage& connectAddr, SocketError* res) {
    (*res) = co_await ConnectAwaiter(this, connectAddr);
}

SocketError SocketImpl::connect(const SockAddr& sockAddr) {
    if (fd == -1) {
        return SocketError::NotInitialized;
    }
    
    SocketError socketError = initSocket(sockAddr.getFamily());
    if (socketError != SocketError::Ok) {
        return socketError;
    }

    sockaddr_storage connectAddr;
    sockAddr.toNative(&connectAddr);

    SocketError socketError;
    connect_co(connectAddr, &res).get();
    return socketError;
}

std::future<void> SocketImpl::read_co(iovec* iov, int iovcnt, std::tuple<uint32_t, SocketError>* res) {
    (*res) = co_await ReadAwaiter(this, iov, iovcnt);
}

std::tuple<uint32_t, SocketError> SocketImpl::read(char* buffer, uint32_t bufferLen) {
    INet::IoBuffer ioBuffer;
    ioBuffer.buffer = buffer;
    ioBuffer.bufferLen = bufferLen;
    return readv(&ioBuffer, 1);
}

std::tuple<uint32_t, SocketError> SocketImpl::readv(INet::IoBuffer* buffers, uint32_t bufferCount) {
    if (fd == -1) {
        return std::make_tuple(0, SocketError::NotInitialized);
    }
    if (bufferCount > INT_MAX) {
        return std::make_tuple(0, SocketError::InvalidArgument);
    }

    iovec staticBufs[20];
    std::unique_ptr<iovec[]> dynamicBufs;
    iovec* usableBuf = nullptr;

    if (bufferCount <= 20) {
        usableBuf = staticBufs;
    } else {
        dynamicBufs = std::make_unique(new iovec[bufferCount]);
        usableBuf = dynamicBufs.get();
    }
    
    std::tuple<uint32_t, SocketError> res(0, SocketError::Ok);
    read_co(usableBuf, (int)bufferCount, &res).get();
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
    if (fd == -1) {
        return SocketError::NotInitialized;
    }
    if (bufferCount > INT_MAX) {
        return std::make_tuple(0, SocketError::InvalidArgument);
    }
    
    iovec staticBufs[20];
    std::unique_ptr<iovec[]> dynamicBufs;
    iovec* usableBuf = nullptr;
    
    if (bufferCount <= 20) {
        usableBuf = staticBufs;
    } else {
        dynamicBufs = std::make_unique(new iovec[bufferCount]);
        usableBuf = dynamicBufs.get();
    }

    SocketError res = SocketError::Ok;
    write_co(usableBuf, (int)bufferCount, &res).get();
    return res;
}

SocketError SocketImpl::shutdownRead() {
    if (fd == -1) {
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
    if (fd == -1) {
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
    if (fd == -1) {
        return SocketError::NotInitialized;
    }
    
    if (bufferSize > INT_MAX) {
        bufferSize = INT_MAX;
    }
    
    int bufferOpt = (int)bufferSize;
    int setOptRes = ::setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char*)&bufferOpt, sizeof(int));
    if (setOptRes != 0) {
        return getSocketError(errno);
    }
    
    return SocketError::Ok;
}

SocketError SocketImpl::setWriteBuf(uint32_t bufferSize) {
    if (fd == -1) {
        return SocketError::NotInitialized;
    }
    
    if (bufferSize > INT_MAX) {
        bufferSize = INT_MAX;
    }
    
    int bufferOpt = (int)bufferSize;
    int setOptRes = ::setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char*)&bufferOpt, sizeof(int));
    if (setOptRes != 0) {
        return getSocketError(errno);
    }
    
    return SocketError::Ok;
}

#endif // __APPLE__
