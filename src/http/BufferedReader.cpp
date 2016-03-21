
#include "cupcake_priv/http/BufferedReader.h"

#include <algorithm>
#include <cassert>

using namespace Cupcake;

BufferedReader::BufferedReader() :
    index(0),
    lineEndIndex(0),
    socket(nullptr)
{}

void BufferedReader::init(Socket* readSocket, size_t initialBufferSize) {
    socket = readSocket;
    buffer.resize(initialBufferSize);
}

SocketError BufferedReader::read() {
    assert(index == 0);

    uint32_t bytesRead;
    SocketError err;
    std::tie(bytesRead, err) = socket->read(buffer.data(), buffer.size());

    index = bytesRead;
    return err;
}

SocketError BufferedReader::readLine(size_t maxLength) {
    assert(index == 0);

    bool foundLineEnd = false;

    do {
        // Increase buffer size if needed
        if (available() == 0) {
            if (buffer.size() == maxLength) {
                return SocketError::Unknown; // TODO: Need to change to HttpErrors
            }

            size_t newSize = std::max(buffer.size() * 2, maxLength+2); // +2 to allow for \r\n
            buffer.reserve(newSize);
        }

        // Read some data
        uint32_t bytesRead;
        SocketError err;
        std::tie(bytesRead, err) = socket->read(buffer.data() + index, available());

        if (err != SocketError::Ok) {
            return err;
        }

        // Look for a newline
        for (size_t i = index; i < buffer.size(); i++) {
            if (buffer[i] == '\n') {
                if (i != 0 && buffer[i - 1] == '\r') {
                    lineEndIndex = i - 1;
                } else {
                    lineEndIndex = i;
                }
                foundLineEnd = true;
            }
        }

        index += bytesRead;

    } while (!foundLineEnd);

    return SocketError::Ok;
}

SocketError BufferedReader::readAtLeast(size_t byteCount) {
    assert(index == 0);

    return SocketError::Ok;
}

void BufferedReader::clear() {
    index = 0;
    lineEndIndex = 0;
}

size_t BufferedReader::available() const {
    return buffer.size() - index;
}

size_t BufferedReader::lineEnd() const {
    return lineEndIndex;
}

const char* BufferedReader::data() const {
    return buffer.data();
}
