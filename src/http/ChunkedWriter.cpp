
#include "cupcake_priv/http/ChunkedWriter.h"

#include "cupcake/net/INet.h"
#include "cupcake_priv/text/Strconv.h"

using namespace Cupcake;

ChunkedWriter::ChunkedWriter(StreamSource* streamSource) :
    streamSource(streamSource),
    closed(false)
{}

ChunkedWriter::~ChunkedWriter() {
    close();
}

std::tuple<uint32_t, HttpError> ChunkedWriter::write(const char* buffer, uint32_t bufferLen) {
    if (bufferLen == 0) {
        return std::make_tuple(0, HttpError::Ok);
    }

    // TODO: Should probably buffer the small writes
    char lengthBuffer[32];
    size_t hexLen = Strconv::uint32ToStr(bufferLen, 16, lengthBuffer, sizeof(lengthBuffer));

    INet::IoBuffer writeBufs[3];
    writeBufs[0].buffer = lengthBuffer;
    writeBufs[0].bufferLen = (uint32_t)hexLen;
    writeBufs[1].buffer = "\r\n";
    writeBufs[1].bufferLen = 2;
    writeBufs[2].buffer = (char*)buffer;
    writeBufs[2].bufferLen = bufferLen;

    return streamSource->writev(writeBufs, 3);
}

HttpError ChunkedWriter::flush() {
    return HttpError::Ok;
}

HttpError ChunkedWriter::close() {
    if (closed) {
        return HttpError::Ok;
    }
    closed = true;

    uint32_t bytesWritten;
    HttpError err;
    std::tie(bytesWritten, err) = streamSource->write("0\r\n\r\n", 5);
    return err;
}
