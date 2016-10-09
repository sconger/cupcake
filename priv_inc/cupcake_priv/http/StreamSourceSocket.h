
#ifndef CUPCAKE_STREAM_SOURCE_SOCKET
#define CUPCAKE_STREAM_SOURCE_SOCKET

#include "cupcake/net/Socket.h"
#include "cupcake_priv/http/StreamSource.h"

namespace Cupcake {

/*
 * Plain socket implementation of StreamSource.
 */
class StreamSourceSocket : public StreamSource {
public:
    StreamSourceSocket(Socket&& socket);
    ~StreamSourceSocket();

    std::tuple<StreamSource*, HttpError> accept() override;
    std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen) override;
    std::tuple<uint32_t, HttpError> readv(INet::IoBuffer* buffers, uint32_t bufferCount) override;
    HttpError write(const char* buffer, uint32_t bufferLen) override;
    HttpError writev(const INet::IoBuffer* buffers, uint32_t bufferCount) override;
    HttpError close() override;

private:
    StreamSourceSocket(const StreamSourceSocket&) = delete;
    StreamSourceSocket& operator=(const StreamSourceSocket&) = delete;

    Socket socket;
};

}

#endif // CUPCAKE_STREAM_SOURCE_SOCKET
