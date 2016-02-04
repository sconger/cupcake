// cupcake_priv_win.h

#ifndef CUPCAKE_PRIV_WIN
#define CUPCAKE_PRIV_WIN

#ifndef _WIN32
#error "This file should only be included in a Windows build"
#endif

#include <Winsock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>
#include <Mstcpip.h>
#include <Mswsock.h>

namespace PrivWin {
    HANDLE getCompletionPort();

    bool isGetAddrInfoExAsyncSupported();

    // Global function pointers to extension functions
    LPFN_ACCEPTEX getAcceptEx();
    LPFN_GETACCEPTEXSOCKADDRS getGetAcceptExSockaddrs();
    LPFN_CONNECTEX getConnectEx();
}

#endif // CUPCAKE_PRIV_WIN
