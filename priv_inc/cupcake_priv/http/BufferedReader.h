
#ifndef CUPCAKE_BUFFERED_READER_H
#define CUPCAKE_BUFFERED_READER_H

#include "cupcake/text/StringRef.h"

#include "cupcake_priv/http/StreamSource.h"

#include <memory>
#include <tuple>

namespace Cupcake {

/*
 * Buffered reader to simplify reading lines or hunks of known-size data. Holds
 * a reference to a socket, so it needs to be destroyed before the socket.
 */
class BufferedReader {
public:
    BufferedReader();
    ~BufferedReader() = default;

    void init(StreamSource* socket, size_t initialBuffer);

    std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen);
    HttpError readFixedLength(char* buffer, uint32_t byteCount);
    std::tuple<StringRef, HttpError> readLine(uint32_t maxLength);

private:
    BufferedReader(const BufferedReader&) = delete;
    BufferedReader& operator=(const BufferedReader&) = delete;

    StreamSource* socket;
    std::unique_ptr<char[]> buffer;
    uint32_t bufferLen;
    uint32_t startIndex;
    uint32_t endIndex;
};

}

#endif // CUPCAKE_BUFFERED_READER_H
