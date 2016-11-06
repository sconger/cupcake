
#ifndef _WIN32

#include "cupcake/net/Addrinfo.h"
#include "cupcake_priv/CString.h"

#include <cassert>
#include <cstdio>
#include <memory>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace Cupcake {

static
SocketError getAddrInfoError(int err) {
    switch (err) {
        case EAI_BADFLAGS:
        case EAI_BADHINTS:
        case EAI_FAMILY:
        case EAI_OVERFLOW:
        case EAI_SERVICE:
        case EAI_SOCKTYPE:
            return SocketError::InvalidArgument;
        case EAI_FAIL:
            return SocketError::AddrFail;
        case EAI_MEMORY:
            return SocketError::OutOfMemory;
        case EAI_NONAME:
            return SocketError::AddrNoName;
        case EAI_AGAIN:
        case EAI_PROTOCOL:
            return SocketError::Unknown;
        case EAI_SYSTEM:
        default:
            return SocketError::Ok; // Use errno
    }
}

static
SocketError getSystemError(int err) {
    // TODO: Not clear when this happens
    return SocketError::Unknown;
}

namespace Addrinfo {
    
SockAddr getAddrAny(INet::Protocol family) {
    assert(family != INet::Protocol::Unknown);
    
    sockaddr_storage storage;
    std::memset(&storage, 0, sizeof(sockaddr_storage));
    
    if (family == INet::Protocol::Ipv4) {
        sockaddr_in* addrin = (sockaddr_in*)&storage;
        addrin->sin_family = AF_INET;
        addrin->sin_addr.s_addr = INADDR_ANY;
    } else {
        sockaddr_in6* addrin = (sockaddr_in6*)&storage;
        addrin->sin6_family = AF_INET6;
        addrin->sin6_addr = IN6ADDR_ANY_INIT;
    }
    
    return SockAddr::fromNative(&storage);
}

SockAddr getLoopback(INet::Protocol family, uint16_t port) {
    assert(family != INet::Protocol::Unknown);
    
    sockaddr_storage storage;
    std::memset(&storage, 0, sizeof(sockaddr_storage));
    
    if (family == INet::Protocol::Ipv4) {
        sockaddr_in* addrin = (sockaddr_in*)&storage;
        addrin->sin_family = AF_INET;
        addrin->sin_port = htons(port);
        addrin->sin_addr.s_addr = INADDR_LOOPBACK;
    } else {
        sockaddr_in6* addrin = (sockaddr_in6*)&storage;
        addrin->sin6_family = AF_INET6;
        addrin->sin6_port = htons(port);
        addrin->sin6_flowinfo = 0;
        addrin->sin6_addr = IN6ADDR_LOOPBACK_INIT;
        addrin->sin6_scope_id = 0;
    }
    
    return SockAddr::fromNative(&storage);
}

SocketError parseNumeric(const StringRef name, uint16_t port, SockAddr* dest) {
    CStringBuf cName(name);
    
    char portBuf[6];
    ::sprintf(portBuf, "%hu", port);
    
    
    addrinfo hints;
    std::memset(&hints, 0, sizeof(addrinfo));
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    addrinfo* resInfo = nullptr;

    int res = ::getaddrinfo(cName.get(), portBuf, &hints, &resInfo);

    if (res != 0) {
        (*dest) = SockAddr();
        SocketError sockErr = getAddrInfoError(res);
        if (sockErr == SocketError::Ok) {
            sockErr = getSystemError(errno);
        }
        return sockErr;
    }
    
    (*dest) = SockAddr::fromNative((const sockaddr_storage*)resInfo->ai_addr);
    ::freeaddrinfo(resInfo);
    return SocketError::Ok;
}

SocketError addrLookup(SockAddr* dest,
                       const StringRef name,
                       uint16_t port,
                       INet::Protocol prot) {
    CStringBuf cName(name);
    
    char portBuf[6];
    ::sprintf(portBuf, "%hu", port);
    
    addrinfo hints;
    std::memset(&hints, 0, sizeof(addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    if (prot == INet::Protocol::Ipv4) {
        hints.ai_family = AF_INET;
    } else {
        hints.ai_family = AF_INET6;
    }
    
    addrinfo* resInfo = nullptr;
    
    int res = ::getaddrinfo(cName.get(), portBuf, &hints, &resInfo);
    
    if (res != 0) {
        (*dest) = SockAddr();
        (*dest) = SockAddr();
        SocketError sockErr = getAddrInfoError(res);
        if (sockErr == SocketError::Ok) {
            sockErr = getSystemError(errno);
        }
        return sockErr;
    }
    
    (*dest) = SockAddr::fromNative((const sockaddr_storage*)resInfo->ai_addr);
    ::freeaddrinfo(resInfo);
    return SocketError::Ok;
}
    
} // End namespace Addrinfo
    
} // End namespace Cupcake

#endif // !_WIN32
