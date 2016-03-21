
#include "unit/net/INet_test.h"
#include "unit/unittest.h"

#include "cupcake/net/INet.h"

using namespace Cupcake;

bool test_inet_gethostname() {
    // Just confirm that we get something back
    String hostname;
    SocketError err;
    std::tie(hostname, err) = INet::getHostName();
    
    if (err != SocketError::Ok) {
        testf("INet::getHostName() failed with %u", err);
    }

    if (hostname.length() == 0) {
        testf("INet::getHostName() returned an empty string");
        return false;
    }
    return true;
}
