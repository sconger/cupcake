
#ifndef _WIN32
#error "This file should only be included in a Windows build"
#endif

#include "cupcake/net/SockAddr.h"

#include <cassert>
#include <cstdio>
#include <cstring>

#include <Ws2tcpip.h>
#include <Mstcpip.h>


SockAddr::SockAddr() {
    std::memset(&storage, 0, sizeof(SOCKADDR_STORAGE));
}

SockAddr::SockAddr(const SockAddr& other) {
    std::memcpy(&storage, &other.storage, sizeof(SOCKADDR_STORAGE));
}

SockAddr& SockAddr::operator=(const SockAddr& other) {
    std::memmove(&storage, &other.storage, sizeof(SOCKADDR_STORAGE));
    return *this;
}

int SockAddr::getFamily() const {
    return storage.ss_family;
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
        return !!IN4ADDR_ISANY(addr);
    } else {
        sockaddr_in6* addr = (sockaddr_in6*)&storage;
        return !!IN6ADDR_ISANY(addr);
    }
}

bool SockAddr::isLoopback() const {
    if (storage.ss_family == AF_INET) {
        sockaddr_in* addr = (sockaddr_in*)&storage;
        return !!IN4ADDR_ISLOOPBACK(addr);
    } else {
        sockaddr_in6* addr = (sockaddr_in6*)&storage;
        return !!IN6ADDR_ISLOOPBACK(addr);
    }
}

String SockAddr::toString() const {
    wchar_t wbuffer[64]; // Arbitrary, but should be large enough
    char buffer[256];
    DWORD len = sizeof(wbuffer);

    int toStrRes = ::WSAAddressToStringW((SOCKADDR*)&storage,
        sizeof(SOCKADDR_STORAGE),
        NULL,
        wbuffer,
        &len);

    if (toStrRes != 0) {
        // TODO: log
        ::DebugBreak();
    }

    int convRes = ::WideCharToMultiByte(CP_UTF8,
        WC_ERR_INVALID_CHARS,
        wbuffer,
        len,
        buffer,
        sizeof(buffer),
        NULL,
        NULL);

    if (convRes == 0) {
        // TODO: log
        ::DebugBreak();
    }

    return String(buffer, convRes);
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
        return std::memcmp(&addrin->sin_addr,
            &other_addrin->sin_addr,
            sizeof(IN_ADDR)) == 0;
    } else {
        sockaddr_in6* addrin = (sockaddr_in6*)&storage;
        sockaddr_in6* other_addrin = (sockaddr_in6*)&storage;
        if (addrin->sin6_port != other_addrin->sin6_port) {
            return false;
        }
        return std::memcmp(&addrin->sin6_addr,
            &other_addrin->sin6_addr,
            sizeof(IN6_ADDR)) == 0;
    }
}

bool SockAddr::operator!=(const SockAddr& other) {
    if (storage.ss_family != other.storage.ss_family) {
        return true;
    }

    if (storage.ss_family == AF_INET) {
        sockaddr_in* addrin = (sockaddr_in*)&storage;
        sockaddr_in* other_addrin = (sockaddr_in*)&storage;
        if (addrin->sin_port != other_addrin->sin_port) {
            return true;
        }
        return std::memcmp(&addrin->sin_addr,
            &other_addrin->sin_addr,
            sizeof(IN_ADDR)) != 0;
    } else {
        sockaddr_in6* addrin = (sockaddr_in6*)&storage;
        sockaddr_in6* other_addrin = (sockaddr_in6*)&storage;
        if (addrin->sin6_port != other_addrin->sin6_port) {
            return true;
        }
        return std::memcmp(&addrin->sin6_addr,
            &other_addrin->sin6_addr,
            sizeof(IN6_ADDR)) != 0;
    }
}

SockAddr SockAddr::fromNative(const SOCKADDR_STORAGE* storagePtr) {
    SockAddr res;
    std::memcpy(&res.storage, storagePtr, sizeof(SOCKADDR_STORAGE));
    return res;
}

void SockAddr::toNative(SOCKADDR_STORAGE* dest) const {
    std::memcpy(dest, &storage, sizeof(SOCKADDR_STORAGE));
}
