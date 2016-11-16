
#ifndef CUPCAKE_CONTENT_LENGTH_READER
#define CUPCAKE_CONTENT_LENGTH_READER

#include "cupcake/http/Http.h"

#include "cupcake/internal/http/BufferedReader.h"

namespace Cupcake {

class ContentLengthReader : public HttpInputStream {
public:
    ContentLengthReader(BufferedReader& bufReader, uint64_t contentLength);
    ~ContentLengthReader() = default;

    std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen) override;
    HttpError close() override;

private:
    ContentLengthReader(const ContentLengthReader&) = delete;
    ContentLengthReader& operator=(const ContentLengthReader&) = delete;

    BufferedReader& bufReader;
    uint64_t contentLength;
};

}

#endif // CUPCAKE_CONTENT_LENGTH_READER
