// sockaddr.h

#ifndef CUPCAKE_SOCKADDR_H
#define CUPCAKE_SOCKADDR_H

#include "cupcake/net/Inet.h"
#include "cupcake/text/String.h"
#include "cupcake/text/StringRef.h"

#ifdef _WIN32
#define NOMINMAX
#include <Winsock2.h>
#include <Windows.h>
#else
#include <sys/socket.h>
#endif

namespace Cupcake {

/*
 * Represents an IPv4 or IPv6 address.
 */
class SockAddr
{
public:
    SockAddr();
    ~SockAddr() = default;

    SockAddr(const SockAddr& other);
    SockAddr& operator=(const SockAddr& other);

    String toString() const;

    INet::Protocol getFamily() const;
    uint16_t getPort() const;

    bool isAddrAny() const;
    bool isLoopback() const;

    bool operator==(const SockAddr& other);
    bool operator!=(const SockAddr& other);

#ifdef WIN32
    static SockAddr fromNative(const SOCKADDR_STORAGE* storage);
    void toNative(SOCKADDR_STORAGE* dest) const;
#else
    static SockAddr fromNative(const sockaddr_storage* storage);
    void toNative(sockaddr_storage* dest) const;
#endif

private:
#ifdef WIN32
    SOCKADDR_STORAGE storage;
#else
    sockaddr_storage storage;
#endif
};

}

#endif // CUPCAKE_SOCKADDR_H
