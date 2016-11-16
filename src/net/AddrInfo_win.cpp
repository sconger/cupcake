
#ifdef _WIN32

#include "cupcake/internal/net/AddrInfo.h"

#include "cupcake/internal/Cstring.h"
#include "cupcake/internal/Cupcake_priv_win.h"

#include <cassert>
#include <cstdio>
#include <memory>

#include <Winsock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>

namespace Cupcake {

static
SocketError getAddrInfoError(int wsaErr) {
    switch (wsaErr) {
        case WSA_NOT_ENOUGH_MEMORY:
            return SocketError::OutOfMemory;
        case WSAEAFNOSUPPORT:
        case WSAEINVAL:
        case WSAESOCKTNOSUPPORT:
        case WSATYPE_NOT_FOUND:
        case WSASERVICE_NOT_FOUND:
            return SocketError::InvalidArgument;
        case WSAHOST_NOT_FOUND:
            return SocketError::AddrNoName;
        case WSANO_DATA:
            return SocketError::AddrNoData;
        case WSANO_RECOVERY:
            return SocketError::AddrFail;
        case WSANOTINITIALISED:
            return SocketError::NotInitialized;
        case WSATRY_AGAIN: // Fallthrough
        default:
            return SocketError::Unknown;
    }
}

struct lookupInfo {
    void* context;
    String name;
    uint16_t port;
    INet::Protocol prot;
    Addrinfo::lookupCallback callback;
};

static
void CALLBACK addrInfoCallback(PTP_CALLBACK_INSTANCE instance,
                               void* context,
                               PTP_WORK work) {
    std::unique_ptr<lookupInfo> uniquePtr((lookupInfo*)context);
    lookupInfo* lookupInfo = uniquePtr.get();

    // Defer to the blocking variant
    SockAddr resAddr;
    SocketError err = Addrinfo::addrLookup(&resAddr, lookupInfo->name, lookupInfo->port, lookupInfo->prot);
    lookupInfo->callback(lookupInfo->context, resAddr, err);
}

namespace Addrinfo {

SockAddr getAddrAny(INet::Protocol family) {
    assert(family != INet::Protocol::Unknown);

    sockaddr_storage storage;

    if (family == INet::Protocol::Ipv4) {
        sockaddr_in* addrin = (sockaddr_in*)&storage;
        IN4ADDR_SETANY(addrin);
    } else {
        sockaddr_in6* addrin = (sockaddr_in6*)&storage;
        IN6ADDR_SETANY(addrin);
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
        addrin->sin_addr = IN4ADDR_LOOPBACK_INIT;
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
    WCStringBuf wideName(name);

    if (wideName.error()) {
        (*dest) = SockAddr();
        return SocketError::InvalidText;
    }

    wchar_t portBuf[6];
    ::wsprintfW(portBuf, L"%hu", port);

    ADDRINFOEXW hints;
    std::memset(&hints, 0, sizeof(ADDRINFOEXW));
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    ADDRINFOEXW* resInfo = nullptr;

    int res = ::GetAddrInfoExW(wideName.get(),
        portBuf,
        NS_DNS,
        NULL,
        &hints,
        &resInfo,
        NULL,
        NULL,
        NULL,
        NULL);

    if (res != 0) {
        (*dest) = SockAddr();
        return getAddrInfoError(::WSAGetLastError());
    }

    (*dest) = SockAddr::fromNative((const sockaddr_storage*)resInfo->ai_addr);
    ::FreeAddrInfoExW(resInfo);
    return SocketError::Ok;
}

SocketError addrLookup(SockAddr* dest,
                       const StringRef name,
                       uint16_t port,
                       INet::Protocol prot) {
    WCStringBuf wideName(name);

    if (wideName.error()) {
        (*dest) = SockAddr();
        return SocketError::InvalidText;
    }

    wchar_t portBuf[6];
    ::wsprintfW(portBuf, L"%hu", port);

    ADDRINFOEXW hints;
    std::memset(&hints, 0, sizeof(ADDRINFOEXW));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (prot == INet::Protocol::Ipv4) {
        hints.ai_family = AF_INET;
    } else {
        hints.ai_family = AF_INET6;
    }

    ADDRINFOEXW* resInfo = nullptr;

    int res = ::GetAddrInfoExW(wideName.get(),
        portBuf,
        NS_DNS,
        NULL,
        &hints,
        &resInfo,
        NULL,
        NULL,
        NULL,
        NULL);

    if (res != 0) {
        (*dest) = SockAddr();
        return getAddrInfoError(::WSAGetLastError());
    }

    (*dest) = SockAddr::fromNative((const sockaddr_storage*)resInfo->ai_addr);
    ::FreeAddrInfoExW(resInfo);
    return SocketError::Ok;
}

SocketError asyncLookup(void* context,
                        const StringRef name,
                        uint16_t port,
                        INet::Protocol prot,
                        const lookupCallback& callback) {

    // If asynchronous GetAddrInfoEx is supported, call it directly
    //if (Iocp::isGetAddrInfoExAsyncSupported()) {
        // TODO: Implement
    //} else {
        // Submit as a thread pool task
        std::unique_ptr<lookupInfo> uniquePtr(
            new lookupInfo());
        uniquePtr.get()->context = context;
        uniquePtr.get()->name = String(name);
        uniquePtr.get()->port = port;
        uniquePtr.get()->prot = prot;
        uniquePtr.get()->callback = callback;

        PTP_WORK ptpWork = ::CreateThreadpoolWork(addrInfoCallback,
                (void*)uniquePtr.get(),
                NULL);

        if (ptpWork == NULL) {
            // TODO: Log errors
            return SocketError::Unknown;
        }

        ::SubmitThreadpoolWork(ptpWork);
        uniquePtr.release();
    //}

    return SocketError::Ok;
}

} // End namespace Addrinfo

} // End namespace Cupcake

#endif // _WIN32
