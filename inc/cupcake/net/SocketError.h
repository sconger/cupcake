
#ifndef CUPCAKE_ERROR_H
#define CUPCAKE_ERROR_H

#include <cstdint>

enum class SocketError : uint32_t {
    Ok = 0,

    // Generic errors
    InvalidArgument, // EINVAL, ENOPROTOOPT, WSAEINVAL,
    // ERROR_INVALID_PARAMETER, ERROR_BAD_ARGUMENTS
    InvalidHandle, // ENOTSOCK, EBADF, WSAENOTSOCK
    TooManyHandles, // ENFILE, EMFILE
    BadAddress, // EFAULT, WSAEFAULT
    OutOfMemory, // ENOMEM
    NotSupported, // ENOTSUP, EOPNOTSUPP
    AccessDenied, // EACCES, EPERM, EROFS, ERROR_ACCESS_DENIED,
                    // ERROR_CURRENT_DIRECTORY, ERROR_WRITE_PROTECT,
                    // ERROR_SHARING_VIOLATION
    TimedOut, // ETIMEDOUT, ETIME
    NotInitialized,
    InvalidState,
    InvalidText, // ERROR_NO_UNICODE_TRANSLATION

    // Inet errors
    NoBufferSpace, // ENOBUFS, WSAENOBUFS
    AddressNotSupported, // EAFNOSUPPORT
    AddressInUse, // EADDRINUSE
    AddressNotAvailable, // EADDRNOTAVAIL
    AlreadyConnected, // EISCONN
    ArgumentListTooLong, // E2BIG
    ConnectionAborted, // ECONNABORTED, WSAECONNABORTED
    ConnectionAlreadyInProgress, // EALREADY
    ConnectionRefused, // ECONNREFUSED
    ConnectionReset, // ECONNRESET, WSAECONNRESET
    ConnectionShutdown, // ESHUTDOWN, WSAESHUTDOWN
    NotConnected, // ENOTCONN, WSAENOTCONN
    HostUnreachable, // EHOSTUNREACH
    NetworkDown, // ENETDOWN, WSAENETDOWN
    NetworkReset, // ENETRESET, WSAENETRESET
    NetworkUnreachable, // ENETUNREACH
    DestinationAddressRequired, // EDESTADDRREQ
    MessageTooLong, // EMSGSIZE
    ProtocolError, // EPROTO, EPROTONOSUPPORT
    NotBound, // EDESTADDRREQ
    OperationAborted,

    // Address lookup errors
    AddrFamily, // EAI_ADDRFAMILY
    AddrNoName, // EAI_NONAME
    AddrNoData, // EAI_NODATA
    AddrFail, // EAI_FAIL

    // Generic error for unmapped error code
    Unknown
};

#endif // CUPCAKE_ERROR_H
