
#ifndef CUPCAKE_CHUNKED_WRITER
#define CUPCAKE_CHUNKED_WRITER

#include "cupcake/http/Http.h"

#include "cupcake_priv/http/BufferedReader.h"

namespace Cupcake {

class ChunkedWriter : public HttpOutputStream {
public:
    ChunkedWriter(StreamSource* streamSource);
    ~ChunkedWriter();

    std::tuple<uint32_t, HttpError> write(const char* buffer, uint32_t bufferLen) override;
    HttpError flush() override;
    HttpError close() override;

private:
    ChunkedWriter(const ChunkedWriter&) = delete;
    ChunkedWriter& operator=(const ChunkedWriter&) = delete;

    StreamSource* streamSource;
    bool closed;
};

}

#endif // CUPCAKE_CHUNKED_WRITER
