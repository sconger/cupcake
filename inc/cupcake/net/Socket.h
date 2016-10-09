
#ifndef CUPCAKE_SOCKET_H
#define CUPCAKE_SOCKET_H

#include "cupcake/net/SockAddr.h"
#include "cupcake/net/SocketError.h"
#include "cupcake/text/StringRef.h"

#include <tuple>

namespace Cupcake {

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
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(SocketImpl* socketImpl);

    SocketImpl* impl;
};

}

#endif // CUPCAKE_SOCKET_H
