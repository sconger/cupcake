
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
 *
 * Intended to work with socket-like thing, but just sockets for now.
 */
class BufferedReader {
public:
    BufferedReader();
    ~BufferedReader() = default;

    void init(StreamSource* socket, size_t initialBuffer);

    std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen);
    HttpError readAtLeast(size_t byteCount);
    std::tuple<StringRef, HttpError> readLine(size_t maxLength);

    const char* data() const;

private:
    BufferedReader(const BufferedReader&) = delete;
    BufferedReader& operator=(const BufferedReader&) = delete;

    void discardReadLines();

    StreamSource* socket;
    std::unique_ptr<char[]> buffer;
    size_t bufferLen;
    size_t startIndex;
    size_t endIndex;
};

}

#endif // CUPCAKE_BUFFERED_READER_H
