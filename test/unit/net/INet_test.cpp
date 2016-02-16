
#include "unit/net/INet_test.h"
#include "unit/unittest.h"

#include "cupcake/net/INet.h"

bool test_inet_gethostname() {
    // Just confirm that we get something back
    Result<String, SocketError> res = INet::getHostName();
    
    if (!res.ok()) {
        testf("INet::getHostName() failed with %u", res.error());
    }

    if (res.get().length() == 0) {
        testf("INet::getHostName() returned an empty string");
        return false;
    }
    return true;
}
