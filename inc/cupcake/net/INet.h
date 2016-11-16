
#ifndef CUPCAKE_INET_H
#define CUPCAKE_INET_H

#include "cupcake/net/SocketError.h"
#include "cupcake/internal/text/String.h"

#include <tuple>

namespace Cupcake {

namespace INet {

// Generic enum for "IPv4 or IPv6" distinctions
enum class Protocol {
    Unknown,
    Ipv4,
    Ipv6
};

// Buffer to use for vector IO
struct IoBuffer {
    char* buffer;
    uint32_t bufferLen;
};

};

}

#endif // CUPCAKE_INET_H
