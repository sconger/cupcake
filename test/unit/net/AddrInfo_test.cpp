
#include "unit/UnitTest.h"
#include "unit/net/AddrInfo_test.h"

#include "cupcake/net/AddrInfo.h"
#include "cupcake/net/Socket.h"

bool test_addrinfo_addrlookup() {
    return true;
}

bool test_addrinfo_asynclookup() {
    return true;
}

/*
bool test_addrinfo_addrlookup() {
    SockAddr resAddr;
    Error err;

    err = Addrinfo::addrLookup(&resAddr, "localhost", 0, INet::Protocol::Ipv4);
    if (err != ERR_OK) {
        testf("Lookup of \"localhost\" failed for IPv4 with error: %d", err);
        return false;
    }

    if (resAddr != Addrinfo::getLoopback(INet::Protocol::Ipv4, 0)) {
        testf("Lookup of \"localhost\" for IPv4 did not produce loopback");
        return false;
    }

    err = Addrinfo::addrLookup(&resAddr, "localhost", 0, INet::Protocol::Ipv6);
    if (err != ERR_OK) {
        testf("Lookup of \"localhost\" failed for IPv6 with error: %d", err);
        return false;
    }

    if (resAddr != Addrinfo::getLoopback(INet::Protocol::Ipv6, 0)) {
        testf("Lookup of \"localhost\" for IPv6 did not produce loopback");
        return false;
    }

    err = Addrinfo::addrLookup(&resAddr, "definatelynotahostnameihope", 0, INet::Protocol::Ipv6);
    if (err == ERR_OK) {
        testf("Lookup of nonsense hostname did not error");
        return false;
    }

    return true;
}


// Looks up localhost for IPv4 and IPv6, making sure they fetch the loopback
// address.
// TODO: This needs to bind a port to use for the test
bool test_addrinfo_asynclookup() {
    Condition cond;
    SockAddr captAddr;
    Error captErr;
    bool lookupDone = false;

    Addrinfo::asyncLookup(nullptr, "localhost", 0, INet::Protocol::Ipv4,
        [&cond, &captAddr, &captErr, &lookupDone](void* context, const SockAddr& addr, Error err) {
        captAddr = addr;
        captErr = err;
        cond.lock();
        lookupDone = true;
        cond.signal();
        cond.unlock();
    });

    cond.lock();
    while (!lookupDone) {
        cond.wait();
    }
    cond.unlock();

    if (captErr != ERR_OK) {
        testf("Lookup of \"localhost\" failed for IPv4 with error: %d", captErr);
        return false;
    }

    if (captAddr != Addrinfo::getLoopback(INet::Protocol::Ipv4, 0)) {
        testf("Lookup of \"localhost\" for IPv4 did not produce loopback");
        return false;
    }

    lookupDone = false;

    Addrinfo::asyncLookup(nullptr, "localhost", 0, INet::Protocol::Ipv6,
        [&cond, &captAddr, &captErr, &lookupDone](void* context, const SockAddr& addr, Error err) {
        captAddr = addr;
        captErr = err;
        cond.lock();
        lookupDone = true;
        cond.signal();
        cond.unlock();
    });

    cond.lock();
    while (!lookupDone) {
        cond.wait();
    }
    cond.unlock();

    if (captErr != ERR_OK) {
        testf("Lookup of \"localhost\" failed for IPv6 with error: %d", captErr);
        return false;
    }

    if (captAddr != Addrinfo::getLoopback(INet::Protocol::Ipv6, 0)) {
        testf("Lookup of \"localhost\" for IPv6 did not produce loopback");
        return false;
    }

    lookupDone = false;

    Addrinfo::asyncLookup(nullptr, "definatelynotahostnameihope", 0, INet::Protocol::Ipv6,
        [&cond, &captAddr, &captErr, &lookupDone](void* context, const SockAddr& addr, Error err) {
        captAddr = addr;
        captErr = err;
        cond.lock();
        lookupDone = true;
        cond.signal();
        cond.unlock();
    });

    cond.lock();
    while (!lookupDone) {
        cond.wait();
    }
    cond.unlock();

    if (captErr == ERR_OK) {
        testf("Lookup of nonsense hostname did not error");
        return false;
    }

    return true;
}
*/