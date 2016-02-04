
#ifndef _WIN32
#error "This file should only be included in a Windows build"
#endif

#include "cupcake/net/AddrInfo.h"

#include "cupcake_priv/Cstring.h"
#include "cupcake_priv/Cupcake_priv_win.h"

#include <cassert>
#include <cstdio>
#include <memory>

#include <Winsock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>

static
Error getAddrInfoError(int wsaErr) {
    switch (wsaErr) {
        case WSA_NOT_ENOUGH_MEMORY:
            return ERR_OUT_OF_MEMORY;
        case WSAEAFNOSUPPORT:
        case WSAEINVAL:
        case WSAESOCKTNOSUPPORT:
        case WSATYPE_NOT_FOUND:
        case WSASERVICE_NOT_FOUND:
            return ERR_INVALID_ARGUMENT;
        case WSAHOST_NOT_FOUND:
            return ERR_ADDR_NONAME;
        case WSANO_DATA:
            return ERR_ADDR_NODATA;
        case WSANO_RECOVERY:
            return ERR_ADDR_FAIL;
        case WSANOTINITIALISED:
            return ERR_NOT_INITIALIZED;
        case WSATRY_AGAIN: // Fallthrough
        default:
            return ERR_UNKNOWN;
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
    Error err = Addrinfo::addrLookup(&resAddr, lookupInfo->name, lookupInfo->port, lookupInfo->prot);
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
    ::memset(&storage, 0, sizeof(sockaddr_storage));

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

Error parseNumeric(const StringRef name, uint16_t port, SockAddr* dest) {
    WCStringBuf wideName(name);

    if (wideName.error() != ERR_OK) {
        (*dest) = SockAddr();
        return wideName.error();
    }

    wchar_t portBuf[6];
    ::wsprintfW(portBuf, L"%hu", port);

    ADDRINFOEXW hints;
    ::memset(&hints, 0, sizeof(ADDRINFOEXW));
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
    return ERR_OK;
}

Error addrLookup(SockAddr* dest,
                 const StringRef name,
                 uint16_t port,
                 INet::Protocol prot) {
    WCStringBuf wideName(name);

    if (wideName.error() != ERR_OK) {
        (*dest) = SockAddr();
        return wideName.error();
    }

    wchar_t portBuf[6];
    ::wsprintfW(portBuf, L"%hu", port);

    ADDRINFOEXW hints;
    ::memset(&hints, 0, sizeof(ADDRINFOEXW));
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
    return ERR_OK;
}

Error asyncLookup(void* context,
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
            return ERR_UNKNOWN;
        }

        ::SubmitThreadpoolWork(ptpWork);
        uniquePtr.release();
    //}

    return ERR_OK;
}

} // End namespace Addrinfo
