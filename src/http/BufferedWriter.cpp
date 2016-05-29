
#include "cupcake_priv/http/BufferedWriter.h"

using namespace Cupcake;

BufferedWriter::BufferedWriter() :
    streamSource(nullptr),
    index(0),
    bufferSize(0)
{}

void BufferedWriter::init(StreamSource* initStreamSource, size_t initBufferSize) {
    streamSource = initStreamSource;
    bufferSize = bufferSize;
    buffer.reset(new char[bufferSize]);
}

std::tuple<uint32_t, HttpError> BufferedWriter::write(const char* writeBuf, uint32_t bufferLen) {
    size_t avail = bufferSize - index;

    // If it can fit in the buffer, copy it in
    if (avail < bufferLen) {
        std::memcpy(buffer.get() + index, writeBuf, bufferLen);
        index += bufferLen;
        return std::make_tuple(bufferLen, HttpError::Ok);
    }

    // If it can't fit, do a write of the existing data, and the new data
    INet::IoBuffer writeBufs[2];
    writeBufs[0].buffer = buffer.get();
    writeBufs[0].bufferLen = index;
    writeBufs[1].buffer = (char*)writeBuf;
    writeBufs[1].bufferLen = bufferLen;

    return streamSource->writev(writeBufs, 2);
}

HttpError BufferedWriter::flush() {
    HttpError err;

    std::tie(std::ignore, err) = streamSource->write(buffer.get(), index);
    if (err == HttpError::Ok) {
        index = 0;
    }
    return err;
}