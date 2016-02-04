
#ifndef _WIN32
#error "This file should only be included in a Windows build"
#endif

#include "cupcake/Cupcake.h"
#include "cupcake_priv/Cupcake_priv_win.h"

#include <Windows.h>

#include <cstdio>

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
        ::printf("WSAIoctl failed when trying to get extension function with: %d\n",
            ::WSAGetLastError());
        ::ExitProcess(1);
    }

    return ptr;
}

namespace Cupcake {

void init() {
    // Initialize Winsock
    WSADATA wsaData;
    int winSockRes = ::WSAStartup(MAKEWORD(2,2), &wsaData);
    if (winSockRes != 0) {
        ::printf("WSAStartup failed with error: %d\n", ::WSAGetLastError());
        ::ExitProcess(1);
    }

    // Global completion port
    completionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE,
        NULL,
        0,
        0);
    if (completionPort == INVALID_HANDLE_VALUE) {
        ::printf("CreateIoCompletionPort failed with: %lu\n", ::GetLastError());
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
        ::printf("WSASocket failed with: %d\n", ::WSAGetLastError());
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
            ::printf("VerifyVersionInfoW failed with: %u\n", verInfoErr);
            ::ExitProcess(1);
        }
    } else {
        isWindows8OrLater = true;
    }
}

void cleanup() {
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

} // End namespace Cupcake

namespace PrivWin {
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
}