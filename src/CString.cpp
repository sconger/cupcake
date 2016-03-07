
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "cupcake_priv/CString.h"

#include <cstring>

CStringBuf::CStringBuf(const StringRef str) {
    size_t strLen = str.length();
    if (strLen < sizeof(statBuf)) {
        strPtr = statBuf;
    } else {
        strPtr = new char[strLen+1];
    }
    std::memcpy(strPtr, str.data(), strLen);
    strPtr[strLen] = '\0';
}

CStringBuf::~CStringBuf() {
    if (strPtr != statBuf) {
        delete[] strPtr;
    }
}

char* CStringBuf::get() const {
    return strPtr;
}

#ifdef _WIN32

#include <Windows.h>

WCStringBuf::WCStringBuf(const StringRef path) :
    errored(false) {

    int res = ::MultiByteToWideChar(CP_UTF8,
        MB_ERR_INVALID_CHARS,
        path.data(),
        (int)path.length(),
        NULL,
        0);

    if (res == 0) {
        // Not worrying for now
        errored = true;
        /*
        int err = ::GetLastError();

        switch (err) {
        case ERROR_INVALID_FLAGS:
        case ERROR_INVALID_PARAMETER:
            errVal = SocketError::InvalidArgument;
            return;
        case ERROR_NO_UNICODE_TRANSLATION:
            errVal = SocketError::InvalidText;
            return;
        default:
            errVal = SocketError::Unknown;
            return;
        }
        */
    }

    int utf16Len = res;
    int bufSize;

    if (utf16Len < sizeof(statBuf)) {
        strPtr = statBuf;
        bufSize = sizeof(statBuf) / sizeof(wchar_t);
    } else {
        bufSize = (int)(utf16Len + 1);
        strPtr = new wchar_t[bufSize];
    }

    int convRes = ::MultiByteToWideChar(CP_UTF8,
        MB_ERR_INVALID_CHARS,
        path.data(),
        (int)path.length(),
        strPtr,
        bufSize);

    if (convRes == 0) {
        // Not worrying for now
        errored = true;
        /*
        int err = ::GetLastError();

        switch (err) {
        case ERROR_INSUFFICIENT_BUFFER:
            errVal = SocketError::NoBufferSpace;
            return;
        case ERROR_INVALID_FLAGS:
        case ERROR_INVALID_PARAMETER:
            errVal = SocketError::InvalidArgument;
            return;
        case ERROR_NO_UNICODE_TRANSLATION:
            errVal = SocketError::InvalidText;
            return;
        default:
            errVal = SocketError::Unknown;
            return;
        }
        */
    }

    strPtr[utf16Len] = L'\0';
}

WCStringBuf::~WCStringBuf() {
    if (strPtr != statBuf) {
        delete[] strPtr;
    }
}

wchar_t* WCStringBuf::get() const {
    return strPtr;
}

bool WCStringBuf::error() const {
    return errored;
}

#define LONG_PREFIX L"\\\\?\\"

WinPathBuf::WinPathBuf(const StringRef path) :
    errored(false) {

    int res = ::MultiByteToWideChar(CP_UTF8,
        MB_ERR_INVALID_CHARS,
        path.data(),
        (int)path.length(),
        NULL,
        0);

    if (res == 0) {
        // Not worrying for now
        errored = true;
        /*
        int err = ::GetLastError();

        switch (err) {
        case ERROR_INVALID_FLAGS:
        case ERROR_INVALID_PARAMETER:
            errVal = SocketError::InvalidArgument;
            return;
        case ERROR_NO_UNICODE_TRANSLATION:
            errVal = SocketError::InvalidText;
            return;
        default:
            errVal = SocketError::Unknown;
            return;
        }
        */
    }

    int utf16Len = res;
    size_t offset = 0;
    int bufSize;

    if (utf16Len < sizeof(statBuf)) {
        pathPtr = statBuf;
        bufSize = sizeof(statBuf) / sizeof(wchar_t);
    } else if (utf16Len < MAX_PATH) {
        bufSize = (int)(utf16Len+1);
        pathPtr = new wchar_t[bufSize];
    } else {
        bufSize = (int)(utf16Len+::wcslen(LONG_PREFIX)+1);
        pathPtr = new wchar_t[bufSize];
        ::wcscpy(pathPtr, LONG_PREFIX); // "\\?\" allows for a longer path
        offset = ::wcslen(LONG_PREFIX);
    }

    int convRes = ::MultiByteToWideChar(CP_UTF8,
        MB_ERR_INVALID_CHARS,
        path.data(),
        (int)path.length(),
        pathPtr + offset,
        bufSize);

    if (convRes == 0) {
        // Not worrying for now
        errored = true;
        /*
        int err = ::GetLastError();

        switch (err) {
        case ERROR_INSUFFICIENT_BUFFER:
            errVal = SocketError::NoBufferSpace;
            return;
        case ERROR_INVALID_FLAGS:
        case ERROR_INVALID_PARAMETER:
            errVal = SocketError::InvalidArgument;
            return;
        case ERROR_NO_UNICODE_TRANSLATION:
            errVal = SocketError::InvalidText;
            return;
        default:
            errVal = SocketError::Unknown;
            return;
        */
    }

    pathPtr[utf16Len+offset] = L'\0';
}

WinPathBuf::~WinPathBuf() {
    if (pathPtr != statBuf) {
        delete[] pathPtr;
    }
}

wchar_t* WinPathBuf::get() const {
    return pathPtr;
}

bool WinPathBuf::error() const {
    return errored;
}

#endif // _WIN32
