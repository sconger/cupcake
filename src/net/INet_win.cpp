
#ifndef _WIN32
#error "This file should only be included in a Windows build"
#endif

#include "cupcake/net/Inet.h"

#include <Winsock2.h>

namespace INet {

Result<String> getHostName() {
    char buffer[256];

    int res = ::gethostname(buffer, sizeof(buffer));

    if (res != 0) {
        int wsaErr = ::WSAGetLastError();
        Error err;

        switch (wsaErr) {
        case WSANOTINITIALISED:
			err = ERR_NOT_INITIALIZED;
            break;
        case WSAENETDOWN:
			err = ERR_NETWORK_DOWN;
            break;
        default:
			err = ERR_UNKNOWN;
        }

        return Result<String>(err);
    }

    return Result<String>(buffer);
}

} // End namespace Inet
