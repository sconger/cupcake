
#ifndef CUPCAKE_SOCKET_IMPL_DARWIN_H
#define CUPCAKE_SOCKET_IMPL_DARWIN_H

#ifdef __APPLE__

#include "cupcake/net/SockAddr.h"

#include <dispatch/dispatch.h>

namespace Cupcake {

/*
 * OS specific socket implementation.
 */
class SocketImpl {
public:
    SocketImpl();
    ~SocketImpl();
    
    SocketError init(INet::Protocol prot);
    SocketError close();
    
    SockAddr getLocalAddress() const;
    SockAddr getRemoteAddress() const;
    
    SocketError bind(const SockAddr& sockAddr);
    SocketError listen();
    SocketError listen(uint32_t queue);
    
    std::tuple<Socket, SocketError> accept();
    SocketError connect(const SockAddr& sockAddr);
    
    std::tuple<uint32_t, SocketError> read(char* buffer, uint32_t bufferLen);
    std::tuple<uint32_t, SocketError> readv(INet::IoBuffer* buffers, uint32_t bufferCount);
    SocketError write(const char* buffer, uint32_t bufferLen);
    SocketError writev(const INet::IoBuffer* buffers, uint32_t bufferCount);
    
    SocketError shutdownRead();
    SocketError shutdownWrite();
    SocketError setReadBuf(uint32_t bufferSize);
    SocketError setWriteBuf(uint32_t bufferSize);
    
private:
    class AcceptAwaiter;
    class ConnectAwaiter;
    class ReadAwaiter;
    class WriteAwaiter;

    SocketError setSocket(int newSocket, int family);
    std::tuple<SocketImpl*, SocketError> tryAccept();
    std::tuple<bool, SocketError> tryConnect(const sockaddr_storage& storage);
    std::tuple<ssize_t, SocketError> tryRead(iovec *iov, int iovcnt);
    std::tuple<ssize_t, SocketError> tryWrite(iovec *iov, int iovcnt);

    int fd;
    dispatch_source_t readSource;
    dispatch_source_t writeSource;
    
    SockAddr localAddr;
    SockAddr remoteAddr;
};


}

#endif // __APPLE__

#endif // CUPCAKE_SOCKET_IMPL_DARWIN_H
