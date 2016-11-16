
#include "cupcake/internal/http/ContentLengthReader.h"

#include <algorithm>
#include <limits>

using namespace Cupcake;

ContentLengthReader::ContentLengthReader(BufferedReader& bufReader, uint64_t contentLength) :
    bufReader(bufReader),
    contentLength(contentLength)
{}

std::tuple<uint32_t, HttpError> ContentLengthReader::read(char* buffer, uint32_t bufferLen) {
    if (contentLength == 0) {
        return std::make_tuple(0, HttpError::Eof);
    }

    uint32_t readLen = std::min((uint32_t)contentLength, bufferLen);

    uint32_t bytesRead;
    HttpError err;
    std::tie(bytesRead, err) = bufReader.read(buffer, readLen);
    if (err == HttpError::Ok) {
        contentLength -= bytesRead;
    }
    return std::make_tuple(bytesRead, err);
}

HttpError ContentLengthReader::close() {
    // TODO: Skip content
    return HttpError::Ok;
}
