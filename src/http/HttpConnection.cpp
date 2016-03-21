
#include "cupcake_priv/http/HttpConnection.h"

using namespace Cupcake;

HttpConnection::HttpConnection(StreamSource* streamSource) :
    streamSource(streamSource)
{}

HttpConnection::~HttpConnection() {
    // TODO
}

void HttpConnection::run() {

}

std::tuple<StringRef, SocketError> HttpConnection::readLine() {
    return std::tuple<StringRef, SocketError>(StringRef(), SocketError::Ok);
}

bool HttpConnection::parseRequestLine() {
    return false;
}

bool HttpConnection::parseHeaderLine() {
    return false;
}

SocketError HttpConnection::sendStatus(uint32_t code, const StringRef reasonPhrase) {
    return SocketError::Ok;
}
