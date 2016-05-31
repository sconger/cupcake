
#ifndef CUPCAKE_BUFFERED_WRITER_H
#define CUPCAKE_BUFFERED_WRITER_H

#include "cupcake/text/StringRef.h"

#include "cupcake_priv/http/StreamSource.h"

#include <memory>
#include <tuple>

namespace Cupcake {

/*
 * Buffered writer to help with writing header data efficiently
 */
class BufferedWriter {
public:
    BufferedWriter();
    ~BufferedWriter() = default;

    void init(StreamSource* streamSource, uint32_t bufferSize);

    std::tuple<uint32_t, HttpError> write(const char* buffer, uint32_t bufferLen);
    HttpError flush();

private:
    BufferedWriter(const BufferedWriter&) = delete;
    BufferedWriter& operator=(const BufferedWriter&) = delete;

    StreamSource* streamSource;
    std::unique_ptr<char[]> buffer;
    uint32_t index;
    uint32_t bufferLen;
};

}

#endif // CUPCAKE_BUFFERED_WRITER_H
