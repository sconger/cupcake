
#include "cupcake/net/Socket.h"

#if defined(__linux__)

#elif defined(__APPLE__)

#elif defined(_WIN32)
#include "cupcake_priv/net/SocketImpl_win.h"
#endif

using namespace Cupcake;

Socket::Socket() : impl(nullptr) {}

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
    impl = new SocketImpl();
    return impl->init(prot);
}

SocketError Socket::close() {
    return impl->close();
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

std::tuple<Socket, SocketError> Socket::accept() {
    return impl->accept();
}

SocketError Socket::connect(const SockAddr& sockAddr) {
    return impl->connect(sockAddr);
}

std::tuple<uint32_t, SocketError> Socket::read(char* buffer, uint32_t bufferLen) {
    return impl->read(buffer, bufferLen);
}

std::tuple<uint32_t, SocketError> Socket::write(const char* buffer, uint32_t bufferLen) {
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

