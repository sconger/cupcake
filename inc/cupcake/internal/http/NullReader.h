
#ifndef CUPCAKE_NULL_READER
#define CUPCAKE_NULL_READER

#include "cupcake/http/Http.h"

namespace Cupcake {

/*
 * When a request has no Content-Length and chunked isn't set, the NullReader is
 * returned for reading from the body. It always indicates EOF.
 */
class NullReader : public HttpInputStream {
public:
    NullReader() = default;
    ~NullReader() = default;

    std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen) override;
    HttpError close() override;

private:
    NullReader(const NullReader&) = delete;
    NullReader& operator=(const NullReader&) = delete;
};

}

#endif // CUPCAKE_CONTENT_LENGTH_READER
