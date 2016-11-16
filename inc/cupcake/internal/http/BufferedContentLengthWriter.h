
#ifndef CUPCAKE_BUFFERED_CONTENT_LENGTH_WRITER
#define CUPCAKE_BUFFERED_CONTENT_LENGTH_WRITER

#include "cupcake/http/Http.h"

#include <vector>

namespace Cupcake {

class HttpResponseImpl;

/*
 * Deals with the case of an HTTP1 request where no content length has been
 * specified on the response. This will buffer the entire response, append
 * that header, and then write out everything in one go.
 *
 * This requires some special support from HttpResponseImpl.
 */
class BufferedContentLengthWriter : public HttpOutputStream {
public:
    BufferedContentLengthWriter();
    ~BufferedContentLengthWriter();

    void init(HttpResponseImpl* responseImpl);

    HttpError write(const char* buffer, uint32_t bufferLen) override;
    HttpError flush() override;
    HttpError close() override;

private:
    BufferedContentLengthWriter(const BufferedContentLengthWriter&) = delete;
    BufferedContentLengthWriter& operator=(const BufferedContentLengthWriter&) = delete;

    HttpResponseImpl* responseImpl;
    std::vector<char> contentBuffer;
    bool closed;
};

}

#endif // CUPCAKE_BUFFERED_CONTENT_LENGTH_WRITER
