
#ifndef CUPCAKE_SOCKET_H
#define CUPCAKE_SOCKET_H

#include "cupcake/Cupcake.h"
#include "cupcake/net/SockAddr.h"
#include "cupcake/net/SocketError.h"
#include "cupcake/text/StringRef.h"

class SocketImpl;

/*
 * Asynchronous socket
 */
class Socket {
public:
    friend class SocketImpl;

    Socket();
    ~Socket();
    Socket(Socket&&);
    Socket& operator=(Socket&&);

    SocketError init(INet::Protocol prot);
    void close();

    SockAddr getLocalAddress() const;
    SockAddr getRemoteAddress() const;

    SocketError bind(const SockAddr& sockAddr);
    SocketError listen();
    SocketError listen(uint32_t queue);

    Result<Socket, SocketError> accept();
    SocketError connect(const SockAddr& sockAddr);

    Result<uint32_t, SocketError> read(char* buffer, uint32_t bufferLen);
    Result<uint32_t, SocketError> write(const char* buffer, uint32_t bufferLen);

    SocketError shutdownRead();
    SocketError shutdownWrite();
    SocketError setReadBuf(uint32_t bufferSize);
    SocketError setWriteBuf(uint32_t bufferSize);

private:
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(SocketImpl* socketImpl);

    SocketImpl* impl;
};

#endif // CUPCAKE_SOCKET_H
