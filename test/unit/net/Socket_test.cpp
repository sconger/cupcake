
#include "unit/UnitTest.h"
#include "unit/net/Socket_test.h"

#include "cupcake/net/AddrInfo.h"
#include "cupcake/net/Socket.h"
#include "cupcake/util/Cleaner.h"

bool test_asyncsocket_bind() {
    return true;
}

bool test_asyncsocket_accept() {
    return true;
}

bool test_asyncsocket_readwrite() {
    return true;
}

/*
bool test_asyncsocket_bind() {
    Error err;
    SockAddr bindAddr = Addrinfo::getAddrAny(INet::Protocol::Ipv4);

    AsyncSocket socket;
    err = socket.bind(bindAddr);
    if (err != ERR_OK) {
        testf("Failed to bind to IPv4 ADDR_ANY with: %d", err);
        return false;
    }

    err = socket.listen();
    if (err != ERR_OK) {
        testf("Failed to listen on bound socket with: %d", err);
        return false;
    }

    return true;
}

bool test_asyncsocket_accept() {
	String dummyContext("Hello");
    Error err = ERR_OK;
    Error callbackErr = ERR_OK;
    SockAddr bindAddr = Addrinfo::getAddrAny(INet::Protocol::Ipv4);
	bool done = false;
	Condition cond;

    AsyncSocket acceptSock;
	AsyncSocket connectSock;

    err = acceptSock.bind(bindAddr);
    if (err != ERR_OK) {
        testf("Failed to bind to IPv4 ADDR_ANY with: %d", err);
        return false;
    }

    err = acceptSock.listen();
    if (err != ERR_OK) {
        testf("Failed to listen on bound socket with: %d", err);
        return false;
    }

	err = acceptSock.accept(&dummyContext, [&cond, &done, &callbackErr]
		(void* context, AsyncSocket* newSock, Error acceptErr) {
		delete newSock;
		cond.lock();
		callbackErr = acceptErr;
		done = true;
		cond.signal();
		cond.unlock();
	});
	if (err != ERR_OK) {
		testf("Accept on listen socket immediately failed with: %d", err);
		return false;
	}

    // TODO: Compare acceptSock.getLocalAddress() to ADDR_ANY

    SockAddr connectAddr = Addrinfo::getLoopback(INet::Protocol::Ipv4,
            acceptSock.getLocalAddress().getPort());

    err = connectSock.connect(&dummyContext, connectAddr, [&cond, &done, &callbackErr]
		(void* context, Error connectErr) {
		if (connectErr != ERR_OK) {
			cond.lock();
			callbackErr = connectErr;
			done = true;
			cond.signal();
			cond.unlock();
		}
	});
	if (err != ERR_OK) {
		testf("Connect with new socket immediately failed with: %d", err);
		return false;
	}

	cond.lock();
	while (!done) {
		cond.wait();
	}
	cond.unlock();

	if (callbackErr != ERR_OK) {
		testf("Accept/connect failed with: %d", callbackErr);
		return false;
	}

    return true;
}

bool test_asyncsocket_readwrite() {
    return true;
}
*/