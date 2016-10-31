
#ifndef _WIN32

#include "cupcake/net/SockAddr.h"

#include <cassert>
#include <cstdio>
#include <cstring>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace Cupcake;

SockAddr::SockAddr() {
    std::memset(&storage, 0, sizeof(sockaddr_storage));
}

SockAddr::SockAddr(const SockAddr& other) {
    std::memcpy(&storage, &other.storage, sizeof(sockaddr_storage));
}

SockAddr& SockAddr::operator=(const SockAddr& other) {
    std::memmove(&storage, &other.storage, sizeof(sockaddr_storage));
    return *this;
}

INet::Protocol SockAddr::getFamily() const {
    if (storage.ss_family == AF_INET) {
        return INet::Protocol::Ipv4;
    } else if (storage.ss_family == AF_INET6) {
        return INet::Protocol::Ipv6;
    } else {
        return INet::Protocol::Unknown;
    }
}

uint16_t SockAddr::getPort() const {
    if (storage.ss_family == AF_INET) {
        sockaddr_in* addr = (sockaddr_in*)&storage;
        return ntohs(addr->sin_port);
    } else {
        sockaddr_in6* addr = (sockaddr_in6*)&storage;
        return ntohs(addr->sin6_port);
    }
}

bool SockAddr::isAddrAny() const {
    if (storage.ss_family == AF_INET) {
        sockaddr_in* addr = (sockaddr_in*)&storage;
        return addr->sin_addr.s_addr == INADDR_ANY;
    } else {
        sockaddr_in6* addr = (sockaddr_in6*)&storage;
        return IN6_IS_ADDR_UNSPECIFIED(&addr->sin6_addr);
    }
}

bool SockAddr::isLoopback() const {
    if (storage.ss_family == AF_INET) {
        sockaddr_in* addr = (sockaddr_in*)&storage;
        return addr->sin_addr.s_addr == INADDR_LOOPBACK;
    } else {
        sockaddr_in6* addr = (sockaddr_in6*)&storage;
        return IN6_IS_ADDR_LOOPBACK(&addr->sin6_addr);
    }
}

String SockAddr::toString() const {
    char buffer[64];
    
    const char* res = inet_ntop(storage.ss_family, buffer,
                                (char*)&storage, (socklen_t)sizeof(sockaddr_storage));
    if (!res) {
        // TODO
    }

    return String(buffer);
}

bool SockAddr::operator==(const SockAddr& other) {
    if (storage.ss_family != other.storage.ss_family) {
        return false;
    }
    
    if (storage.ss_family == AF_INET) {
        sockaddr_in* addrin = (sockaddr_in*)&storage;
        sockaddr_in* other_addrin = (sockaddr_in*)&storage;
        if (addrin->sin_port != other_addrin->sin_port) {
            return false;
        }
        return addrin->sin_addr.s_addr == other_addrin->sin_addr.s_addr;
    } else {
        sockaddr_in6* addrin = (sockaddr_in6*)&storage;
        sockaddr_in6* other_addrin = (sockaddr_in6*)&storage;
        if (addrin->sin6_port != other_addrin->sin6_port) {
            return false;
        }
        return IN6_ARE_ADDR_EQUAL(&addrin->sin6_addr, &other_addrin->sin6_addr);
    }
}

bool SockAddr::operator!=(const SockAddr& other) {
    return !(*this == other);
}

SockAddr SockAddr::fromNative(const sockaddr_storage* storagePtr) {
    SockAddr res;
    std::memcpy(&res.storage, storagePtr, sizeof(sockaddr_storage));
    return res;
}

void SockAddr::toNative(sockaddr_storage* dest) const {
    std::memcpy(dest, &storage, sizeof(sockaddr_storage));
}

#endif // !_WIN32
