
#include "cupcake/internal/http/BufferedWriter.h"

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

HttpError BufferedWriter::write(const char* writeBuf, uint32_t inBufferLen) {
    // If it can fit in the buffer, copy it in
    if (index < bufferLen) {
        std::memcpy(buffer.get() + index, writeBuf, inBufferLen);
        index += inBufferLen;
        return HttpError::Ok;
    }

    // If it can't fit, do a write of the existing data, and the new data
    INet::IoBuffer writeBufs[2];
    writeBufs[0].buffer = buffer.get();
    writeBufs[0].bufferLen = index;
    writeBufs[1].buffer = (char*)writeBuf;
    writeBufs[1].bufferLen = inBufferLen;

    HttpError err = streamSource->writev(writeBufs, 2);
    if (err == HttpError::Ok) {
        index = 0;
    }
    return err;
}

HttpError BufferedWriter::flush() {
    HttpError err = streamSource->write(buffer.get(), index);
    if (err == HttpError::Ok) {
        index = 0;
    }
    return err;
}
