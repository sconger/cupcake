
#ifndef _WIN32

#include "cupcake/net/INet.h"

#include <errno.h>
#include <unistd.h>

namespace Cupcake {
    
namespace INet {
    
std::tuple<String, SocketError> getHostName() {
    char buffer[256];
    
    int res = ::gethostname(buffer, sizeof(buffer));
    
    if (res != 0) {
        int err = errno;
        SocketError sockErr;
        
        switch (err) {
            case EPERM:
                sockErr = SocketError::AccessDenied;
                break;
            default:
                sockErr = SocketError::Unknown;
        }
        
        return std::make_tuple(String(), sockErr);
    }
    
    return std::make_tuple(buffer, SocketError::Ok);
}
    
}
    
}

#endif // !_WIN32
