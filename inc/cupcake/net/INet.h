
#ifndef CUPCAKE_INET_H
#define CUPCAKE_INET_H

#include "cupcake/text/String.h"
#include "cupcake/util/Result.h"

namespace INet
{
    // Generic enum for "IPv4 or IPv6" distinctions
    enum class Protocol
    {
        Unknown,
        Ipv4,
        Ipv6
    };

    Result<String> getHostName();
};

#endif // CUPCAKE_INET_H
