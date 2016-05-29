
#ifndef CUPCAKE_STREAM_SOURCE_H
#define CUPCAKE_STREAM_SOURCE_H

#include "cupcake/http/HttpError.h"
#include "cupcake/net/INet.h"

#include <tuple>

namespace Cupcake {

/*
 * The HTTP logic needs to work for a diverse set of streams: sockets, TLS sockets, unix
 * domain sockets, and possible stdin/stdout for fuzz testing. This is a generic wrapper
 * for some sort of stream so that the HTTP logic can treat them the same.
 *
 * Also allows for unit testing the reader/writer classes.
 */
class StreamSource {
public:
    virtual std::tuple<StreamSource*, HttpError> accept() = 0;
    virtual std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen) = 0;
    virtual std::tuple<uint32_t, HttpError> readv(INet::IoBuffer* buffers, uint32_t bufferCount) = 0;
    virtual std::tuple<uint32_t, HttpError> write(const char* buffer, uint32_t bufferLen) = 0;
    virtual std::tuple<uint32_t, HttpError> writev(const INet::IoBuffer* buffers, uint32_t bufferCount) = 0;
    virtual HttpError close() = 0;
};

}

#endif // CUPCAKE_STREAM_SOURCE_H
