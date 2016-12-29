
#ifdef _WIN32

#include "cupcake/internal/Global_win.h"

#include <Windows.h>

#include <cstdio>

static INIT_ONCE initOnce = INIT_ONCE_STATIC_INIT;
static bool needCleanup = false;

static HANDLE completionPort = INVALID_HANDLE_VALUE;

static bool isWindows8OrLater = false;

static LPFN_ACCEPTEX acceptExPtr = NULL;
static LPFN_GETACCEPTEXSOCKADDRS getAcceptExSockaddrsPtr = NULL;
static LPFN_CONNECTEX connectExPtr = NULL;

static GUID acceptExGUID = WSAID_ACCEPTEX;
static GUID getAcceptExSockaddrsGUID = WSAID_GETACCEPTEXSOCKADDRS;
static GUID connectExGUID = WSAID_CONNECTEX;

/*
* Gets an IO extension based on a GUID. Windows demands you get the
* extended IO functions through the WSAIoctl() interface, but it
* should always succeed for basic TCP/IP as the functions are directly
* exported from a DLL.
*/
static
void* getExtension(SOCKET sock,
    void* guid) {
    void *ptr = NULL;
    DWORD bytes = 0;
    int res = ::WSAIoctl(sock,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        guid,
        sizeof(GUID),
        &ptr,
        sizeof(ptr),
        &bytes,
        NULL,
        NULL);

    if (res == SOCKET_ERROR) {
        std::printf("WSAIoctl failed when trying to get extension function with: %d\n",
            ::WSAGetLastError());
        ::ExitProcess(1);
    }

    return ptr;
}

static
BOOL CALLBACK initOnceCallback(INIT_ONCE* InitOnce, void* Parameter, void** Context) {
    needCleanup = true;

    // Initialize Winsock
    WSADATA wsaData;
    int winSockRes = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (winSockRes != 0) {
        std::printf("WSAStartup failed with error: %d\n", ::WSAGetLastError());
        ::ExitProcess(1);
    }

    // Global completion port
    completionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE,
        NULL,
        0,
        0);
    if (completionPort == INVALID_HANDLE_VALUE) {
        std::printf("CreateIoCompletionPort failed with: %lu\n", ::GetLastError());
        ::ExitProcess(1);
    }

    // Temporary socket to get at extension functions
    SOCKET sock = ::WSASocketW(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP,
        NULL,
        0,
        WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET) {
        std::printf("WSASocket failed with error: %d\n", ::WSAGetLastError());
        ::ExitProcess(1);
    }

    // Fetch extension functions
    acceptExPtr = (LPFN_ACCEPTEX)getExtension(sock, &acceptExGUID);
    getAcceptExSockaddrsPtr = (LPFN_GETACCEPTEXSOCKADDRS)getExtension(sock, &getAcceptExSockaddrsGUID);
    connectExPtr = (LPFN_CONNECTEX)getExtension(sock, &connectExGUID);

    // Close the temp socket
    ::closesocket(sock);

    // Detect windows version, so we can see if GetAddrInfoEx can be asynchronous
    OSVERSIONINFOEXW osvi;
    ::ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 2;

    DWORDLONG condMask = 0;
    condMask = VER_SET_CONDITION(condMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    condMask = VER_SET_CONDITION(condMask, VER_MINORVERSION, VER_GREATER_EQUAL);

    BOOL verInfoRes = ::VerifyVersionInfoW(&osvi,
        VER_MAJORVERSION | VER_MINORVERSION,
        condMask);

    if (!verInfoRes) {
        DWORD verInfoErr = ::GetLastError();

        switch (verInfoErr) {
        case ERROR_OLD_WIN_VERSION: // Normal fail due to version
            break;
        default:
            std::printf("VerifyVersionInfoW failed with: %u\n", verInfoErr);
            ::ExitProcess(1);
        }
    } else {
        isWindows8OrLater = true;
    }

    return TRUE;
}

static
void cleanupGlobals() {
    // Cleanup Winsock
    int cleanupRes = ::WSACleanup();

    if (cleanupRes != 0) {
        ::printf("WSACleanup failed with error: %d\n", ::WSAGetLastError());
    }

    // Cleanup io completion port
    if (completionPort != INVALID_HANDLE_VALUE) {
        ::CloseHandle(completionPort);
        completionPort = INVALID_HANDLE_VALUE;
    }
}

// Helper class to clean things up as the process exits
class GlobalCleaner {
public:
    GlobalCleaner() {}
    ~GlobalCleaner() {
        cleanupGlobals();
    }
};

static
GlobalCleaner globalCleaner;

namespace Cupcake {

namespace Global {

void initGlobals() {
    BOOL initialized = ::InitOnceExecuteOnce(&initOnce, initOnceCallback, nullptr, nullptr);
    if (!initialized) {
        std::printf("InitOnceExecuteOnce failed with error: %u:\n", ::GetLastError());
        ::ExitProcess(1);
    }
}

HANDLE getCompletionPort() {
    return completionPort;
}

bool isGetAddrInfoExAsyncSupported() {
    return isWindows8OrLater;
}

// Global function pointers to extension functions
LPFN_ACCEPTEX getAcceptEx() {
    return acceptExPtr;
}

LPFN_GETACCEPTEXSOCKADDRS getGetAcceptExSockaddrs() {
    return getAcceptExSockaddrsPtr;
}

LPFN_CONNECTEX getConnectEx() {
    return connectExPtr;
}

} // End namespace Global
} // End namespace Cupcake

#endif // _WIN32
