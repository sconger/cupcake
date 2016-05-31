
#include "cupcake_priv/http/BufferedWriter.h"

using namespace Cupcake;

BufferedWriter::BufferedWriter() :
    streamSource(nullptr),
    index(0),
    bufferLen(0)
{}

void BufferedWriter::init(StreamSource* initStreamSource, uint32_t initBufferSize) {
    streamSource = initStreamSource;
    bufferLen = initBufferSize;
    buffer.reset(new char[initBufferSize]);
}

std::tuple<uint32_t, HttpError> BufferedWriter::write(const char* writeBuf, uint32_t inBufferLen) {
    // If it can fit in the buffer, copy it in
    if (index < bufferLen) {
        std::memcpy(buffer.get() + index, writeBuf, inBufferLen);
        index += inBufferLen;
        return std::make_tuple(inBufferLen, HttpError::Ok);
    }

    // If it can't fit, do a write of the existing data, and the new data
    INet::IoBuffer writeBufs[2];
    writeBufs[0].buffer = buffer.get();
    writeBufs[0].bufferLen = index;
    writeBufs[1].buffer = (char*)writeBuf;
    writeBufs[1].bufferLen = inBufferLen;

    uint32_t prevIndex = index;
    uint32_t bytesWritten;
    HttpError err;
    std::tie(bytesWritten, err) = streamSource->writev(writeBufs, 2);
    if (err == HttpError::Ok) {
        index = 0;
    }
    return std::make_tuple(bytesWritten - prevIndex, err);
}

HttpError BufferedWriter::flush() {
    HttpError err;

    std::tie(std::ignore, err) = streamSource->write(buffer.get(), index);
    if (err == HttpError::Ok) {
        index = 0;
    }
    return err;
}