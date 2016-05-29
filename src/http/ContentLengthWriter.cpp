
#include "cupcake_priv/http/ContentLengthWriter.h"

using namespace Cupcake;

ContentLengthWriter::ContentLengthWriter(StreamSource& streamSource) :
    streamSource(streamSource)
{}

ContentLengthWriter::~ContentLengthWriter() {
    close();
}

std::tuple<uint32_t, HttpError> ContentLengthWriter::write(const char* buffer, uint32_t bufferLen) {
    return streamSource.write(buffer, bufferLen);
}

HttpError ContentLengthWriter::close() {
    // TODO: Cause return EOF on future writes?
    return HttpError::Ok;
}