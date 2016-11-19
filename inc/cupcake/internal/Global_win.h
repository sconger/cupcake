
#ifndef CUPCAKE_GLOBAL_WIN
#define CUPCAKE_GLOBAL_WIN

#ifdef _WIN32

#include <Winsock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>
#include <Mstcpip.h>
#include <Mswsock.h>

/*
 * This mess of a namespace handles creation of globals needed by the library.
 * There shouldn't be too many of those, but most OSes require some one time
 * initialization effort.
 *
 * Be sure to call initGlobals before accessing any data contained within.
 */
namespace Cupcake {
namespace Global {

void initGlobals();

HANDLE getCompletionPort();

bool isGetAddrInfoExAsyncSupported();

// Global function pointers to extension functions
LPFN_ACCEPTEX getAcceptEx();
LPFN_GETACCEPTEXSOCKADDRS getGetAcceptExSockaddrs();
LPFN_CONNECTEX getConnectEx();

}
}

#endif // _WIN32

#endif // CUPCAKE_GLOBAL_WIN
