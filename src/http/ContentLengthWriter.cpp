
#include "cupcake_priv/http/ContentLengthWriter.h"

using namespace Cupcake;

ContentLengthWriter::ContentLengthWriter() :
    streamSource(nullptr),
    length(0)
{}

void ContentLengthWriter::init(StreamSource* initStreamSource, uint64_t initLength) {
    streamSource = initStreamSource;
    length = initLength;
}

ContentLengthWriter::~ContentLengthWriter() {
    if (streamSource) {
        close();
    }
}

std::tuple<uint32_t, HttpError> ContentLengthWriter::write(const char* buffer, uint32_t bufferLen) {
    if (bufferLen > length) {
        return std::make_tuple(0, HttpError::ContentLengthExceeded);
    }
    uint32_t bytesWritten;
    HttpError err;
    std::tie(bytesWritten, err) = streamSource->write(buffer, bufferLen);
    if (err == HttpError::Ok) {
        length -= bufferLen;
    }
    return std::make_tuple(bytesWritten, err);
}

HttpError ContentLengthWriter::flush() {
    return HttpError::Ok;
}

HttpError ContentLengthWriter::close() {
    // TODO: Cause return EOF on future writes?
    return HttpError::Ok;
}