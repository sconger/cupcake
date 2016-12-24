
#ifndef CUPCAKE_BUFFERED_READER_H
#define CUPCAKE_BUFFERED_READER_H

#include "cupcake/text/StringRef.h"

#include "cupcake/internal/http/StreamSource.h"

#include <memory>
#include <tuple>

namespace Cupcake {

/*
 * Buffered reader that attempts to handle both line based and fixed length
 * operations in a reasonably performant way, as a connection that goes
 * through the HTTP upgrade will have to do both.
 *
 * Holds a reference to a StreamSource, so it needs to be destroyed before it.
 */
class BufferedReader {
public:
    BufferedReader();
    ~BufferedReader() = default;

    void init(StreamSource* socket, size_t initialBuffer);

    std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen);
    HttpError readFixedLength(char* buffer, uint32_t byteCount);
    std::tuple<bool, HttpError> peekMatch(char* expectedData, uint32_t expectedDataLen);
    std::tuple<StringRef, HttpError> readLine(uint32_t maxLength);
    HttpError discard(uint32_t discardBytes);

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
