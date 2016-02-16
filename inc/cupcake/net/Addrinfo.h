
#ifndef CUPCAKE_ADDRINFO_H
#define CUPCAKE_ADDRINFO_H

#include "cupcake/net/SockAddr.h"
#include "cupcake/net/SocketError.h"
#include "cupcake/text/StringRef.h"
/*
 * Address lookup related functionality
 */
namespace Addrinfo {

typedef std::function<void(void* context, const SockAddr& address, SocketError err)> lookupCallback;

// Returns the any-address (bind to any local interface)
SockAddr getAddrAny(INet::Protocol family);

// Returns the loopback address
SockAddr getLoopback(INet::Protocol family, uint16_t port);

// Parses a numeric socket address
SocketError parseNumeric(const StringRef name, uint16_t port, SockAddr* dest);

// Blocking address lookup
SocketError addrLookup(SockAddr* dest,
                       const StringRef name,
                       uint16_t port,
                       INet::Protocol prot);

// Does an asynchronous address lookup.
SocketError asyncLookup(void* context,
                        const StringRef name,
                        uint16_t port,
                        INet::Protocol prot,
                        const lookupCallback& callback);
}

#endif // CUPCAKE_ADDRINFO_H
