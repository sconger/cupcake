
#include "cupcake/net/Socket.h"

#if defined(__linux__)

#elif defined(__APPLE__)

#elif defined(_WIN32)
#include "cupcake_priv/SocketImpl_win.h"
#endif

Socket::Socket() : impl(new SocketImpl()) {}

Socket::Socket(Socket&& other) {
    impl = other.impl;
    other.impl = nullptr;
}

Socket& Socket::operator=(Socket&& other) {
    if (this != &other) {
        impl = other.impl;
        other.impl = nullptr;
    }
    return *this;
}

Socket::Socket(SocketImpl* socketImpl) : impl(socketImpl) {}

Socket::~Socket() {
    delete impl;
}

SocketError Socket::init(INet::Protocol prot) {
    return impl->init(prot);
}

void Socket::close() {
    impl->close();
}

SockAddr Socket::getLocalAddress() const {
    return impl->getLocalAddress();
}

SockAddr Socket::getRemoteAddress() const {
    return impl->getRemoteAddress();
}

SocketError Socket::bind(const SockAddr& sockAddr) {
    return impl->bind(sockAddr);
}

SocketError Socket::listen() {
    return impl->listen();
}

SocketError Socket::listen(uint32_t queue) {
    return impl->listen(queue);
}

Result<Socket, SocketError> Socket::accept() {
    return impl->accept();
}

SocketError Socket::connect(const SockAddr& sockAddr) {
    return impl->connect(sockAddr);
}

Result<uint32_t, SocketError> Socket::read(char* buffer, uint32_t bufferLen) {
    return impl->read(buffer, bufferLen);
}

Result<uint32_t, SocketError> Socket::write(const char* buffer, uint32_t bufferLen) {
    return impl->write(buffer, bufferLen);
}

SocketError Socket::shutdownRead() {
    return impl->shutdownRead();
}

SocketError Socket::shutdownWrite() {
    return impl->shutdownWrite();
}

SocketError Socket::setReadBuf(uint32_t bufferSize) {
    return impl->setReadBuf(bufferSize);
}

SocketError Socket::setWriteBuf(uint32_t bufferSize) {
    return impl->setWriteBuf(bufferSize);
}

