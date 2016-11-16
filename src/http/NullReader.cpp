
#include "cupcake/internal/http/NullReader.h"

using namespace Cupcake;

std::tuple<uint32_t, HttpError> NullReader::read(char* buffer, uint32_t bufferLen) {
    return std::make_tuple(0, HttpError::Eof);
}

HttpError NullReader::close() {
    return HttpError::Ok;
}