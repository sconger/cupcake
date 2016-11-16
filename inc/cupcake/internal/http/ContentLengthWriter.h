
#ifndef CUPCAKE_CONTENT_LENGTH_WRITER
#define CUPCAKE_CONTENT_LENGTH_WRITER

#include "cupcake/http/Http.h"

#include "cupcake/internal/http/StreamSource.h"

namespace Cupcake {

/*
 * HttpOutputStream for responses where the user has specified a Content-Length.
 *
 * This is basically a straight pass through to the socket, except that it will
 * error if the wrong number of byes are written.
 */
class ContentLengthWriter : public HttpOutputStream {
public:
    ContentLengthWriter();
    ~ContentLengthWriter();

    void init(StreamSource* streamSource, uint64_t length);

    HttpError write(const char* buffer, uint32_t bufferLen) override;
    HttpError flush() override;
    HttpError close() override;

private:
    ContentLengthWriter(const ContentLengthWriter&) = delete;
    ContentLengthWriter& operator=(const ContentLengthWriter&) = delete;

    StreamSource* streamSource;
    uint64_t length;
};

}

#endif // CUPCAKE_CONTENT_LENGTH_WRITER
