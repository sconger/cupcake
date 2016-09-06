
#ifndef CUPCAKE_CHUNKED_READER
#define CUPCAKE_CHUNKED_READER

#include "cupcake/http/Http.h"

#include "cupcake_priv/http/BufferedReader.h"

namespace Cupcake {

/*
 * HttpInputStream for chunked data.
 *
 * TODO: In order to support trailing headers, this probably needs a bufferred version.
 * Doesn't seem to be much use in the wild (no browser support).
 */
class ChunkedReader : public HttpInputStream {
public:
    ChunkedReader(BufferedReader& bufReader);
    ~ChunkedReader() = default;

    std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen) override;
    HttpError close() override;

private:
    ChunkedReader(const ChunkedReader&) = delete;
    ChunkedReader& operator=(const ChunkedReader&) = delete;

    enum class ChunkedState;

    BufferedReader& bufReader;
    uint64_t curLength;
    ChunkedState chunkedState;
};

}

#endif // CUPCAKE_CHUNKED_READER
