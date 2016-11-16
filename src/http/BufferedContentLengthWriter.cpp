
#include "cupcake/internal/http/BufferedContentLengthWriter.h"

#include "cupcake/internal/http/HttpResponseImpl.h"

using namespace Cupcake;

BufferedContentLengthWriter::BufferedContentLengthWriter() :
    responseImpl(nullptr),
    closed(false)
{}

void BufferedContentLengthWriter::init(HttpResponseImpl* initResponseImpl) {
    responseImpl = initResponseImpl;
}

BufferedContentLengthWriter::~BufferedContentLengthWriter() {
    if (responseImpl && !closed) {
        close();
    }
}

HttpError BufferedContentLengthWriter::write(const char* buffer, uint32_t bufferLen) {
    if (closed) {
        return HttpError::StreamClosed;
    }
    contentBuffer.insert(contentBuffer.end(), buffer, buffer+bufferLen);
    return HttpError::Ok;
}

HttpError BufferedContentLengthWriter::flush() {
    return HttpError::Ok;
}

HttpError BufferedContentLengthWriter::close() {
    if (closed) {
        return HttpError::StreamClosed;
    }
    closed = true;
    return responseImpl->writeHeadersAndBody(contentBuffer.data(), contentBuffer.size());
}