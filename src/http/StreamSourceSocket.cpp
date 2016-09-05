
#include "cupcake_priv/http/StreamSourceSocket.h"

using namespace Cupcake;

// TODO: Generally needs more error brains

StreamSourceSocket::StreamSourceSocket(Socket&& socket) :
    socket(std::move(socket))
{}

StreamSourceSocket::~StreamSourceSocket() {
    close();
}

std::tuple<StreamSource*, HttpError> StreamSourceSocket::accept() {
    Socket acceptedSocket;
    SocketError err;
    std::tie(acceptedSocket, err) = socket.accept();

    // TODO: Probably want to push this into the socket implementations
    if (err == SocketError::InvalidHandle ||
        err == SocketError::OperationAborted) {
        return std::make_tuple(nullptr, HttpError::StreamClosed);
    } else if (err != SocketError::Ok) {
        return std::make_tuple(nullptr, HttpError::IoError);
    }

    return std::make_tuple(new StreamSourceSocket(std::move(acceptedSocket)), HttpError::Ok);
}

std::tuple<uint32_t, HttpError> StreamSourceSocket::read(char* buffer, uint32_t bufferLen) {
    uint32_t bytesRead;
    SocketError err;
    std::tie(bytesRead, err) = socket.read(buffer, bufferLen);

    if (err != SocketError::Ok) {
        return std::make_tuple(0, HttpError::IoError);
    }

    if (bytesRead == 0) {
        return std::make_tuple(0, HttpError::Eof);
    }

    return std::make_tuple(bytesRead, HttpError::Ok);
}

std::tuple<uint32_t, HttpError> StreamSourceSocket::readv(INet::IoBuffer* buffers, uint32_t bufferCount) {
    uint32_t bytesRead;
    SocketError err;
    std::tie(bytesRead, err) = socket.readv(buffers, bufferCount);

    if (err != SocketError::Ok) {
        return std::make_tuple(0, HttpError::IoError);
    }

    if (bytesRead == 0) {
        return std::make_tuple(0, HttpError::Eof);
    }

    return std::make_tuple(bytesRead, HttpError::Ok);
}

std::tuple<uint32_t, HttpError> StreamSourceSocket::write(const char* buffer, uint32_t bufferLen) {
    uint32_t bytesWritten;
    SocketError err;
    std::tie(bytesWritten, err) = socket.write(buffer, bufferLen);

    if (err != SocketError::Ok) {
        return std::make_tuple(0, HttpError::IoError);
    }

    return std::make_tuple(bytesWritten, HttpError::Ok);
}

std::tuple<uint32_t, HttpError> StreamSourceSocket::writev(const INet::IoBuffer* buffers, uint32_t bufferCount) {
    uint32_t bytesWritten;
    SocketError err;
    std::tie(bytesWritten, err) = socket.writev(buffers, bufferCount);

    if (err != SocketError::Ok) {
        return std::make_tuple(0, HttpError::IoError);
    }

    return std::make_tuple(bytesWritten, HttpError::Ok);
}

HttpError StreamSourceSocket::close() {
    SocketError err = socket.close();
    if (err != SocketError::Ok) {
        return HttpError::IoError;
    }
    return HttpError::Ok;
}
