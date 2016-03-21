
#ifndef CUPCAKE_BUFFERED_READER_H
#define CUPCAKE_BUFFERED_READER_H

#include "cupcake/net/Socket.h"

#include <tuple>
#include <vector>

namespace Cupcake {

/*
 * Buffered reader to simplify reading lines or hunks of known-size HTTP2 data.
 * Holds a reference to a socket, so it needs to be destroyed before the socket.
 *
 * Intended to work with socket-like thing, but just sockets for now.
 */
class BufferedReader {
public:
    BufferedReader();
    ~BufferedReader() = default;

    void init(Socket* socket, size_t initialB);

    SocketError read();
    SocketError readLine(size_t maxLength);
    SocketError readAtLeast(size_t byteCount);

    void clear();
    size_t available() const;
    size_t lineEnd() const;
    const char* data() const;

private:
    BufferedReader(const BufferedReader&) = delete;
    BufferedReader& operator=(const BufferedReader&) = delete;

    Socket* socket;
    std::vector<char> buffer;
    size_t index;
    size_t lineEndIndex;
};

}

#endif // CUPCAKE_BUFFERED_READER_H
