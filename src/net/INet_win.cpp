
#ifdef _WIN32

#include "cupcake/net/Inet.h"

#include <Winsock2.h>

namespace Cupcake {

namespace INet {

std::tuple<String, SocketError> getHostName() {
    char buffer[256];

    int res = ::gethostname(buffer, sizeof(buffer));

    if (res != 0) {
        int wsaErr = ::WSAGetLastError();
        SocketError err;

        switch (wsaErr) {
        case WSANOTINITIALISED:
            err = SocketError::NotInitialized;
            break;
        case WSAENETDOWN:
            err = SocketError::NetworkDown;
            break;
        default:
            err = SocketError::Unknown;
        }

        return std::make_tuple(String(), err);
    }

    return std::make_tuple(buffer, SocketError::Ok);
}

}

}

#endif // _WIN32
