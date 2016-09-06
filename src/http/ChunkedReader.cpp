
#include "cupcake_priv/http/ChunkedReader.h"

#include "cupcake_priv/text/Strconv.h"

#include <algorithm>

using namespace Cupcake;

enum class ChunkedReader::ChunkedState {
    Length,
    Data,
    Eof
};

ChunkedReader::ChunkedReader(BufferedReader& bufReader) :
    bufReader(bufReader),
    curLength(0),
    chunkedState(ChunkedState::Length)
{}

std::tuple<uint32_t, HttpError> ChunkedReader::read(char* buffer, uint32_t bufferLen) {
    HttpError err;

    // If at the end of stream, return EOF
    if (chunkedState == ChunkedState::Eof) {
        return std::make_tuple(0, HttpError::Eof);
    }

    // If we need to read a length, read it
    if (chunkedState == ChunkedState::Length) {
        StringRef chunkedLine;
        std::tie(chunkedLine, err) = bufReader.readLine(1024); // TODO: Define limit somewhere
        if (err != HttpError::Ok) {
            return std::make_tuple(0, err);
        }
        // Strip and ignore any chunked extension
        ptrdiff_t semiIndex = chunkedLine.indexOf(';');
        if (semiIndex != -1) {
            chunkedLine = chunkedLine.substring(0, semiIndex);
        }

        // Try to parse the hex length number
        bool parseSuccess;
        std::tie(curLength, parseSuccess) = Strconv::parseUint64(chunkedLine, 16);
        if (!parseSuccess) {
            return std::make_tuple(0, HttpError::ClientError);
        }
        if (curLength == 0) {
            // Read the expected blank line or trailing headers. Just skipping trailing headers for now.
            while (true) {
                std::tie(chunkedLine, err) = bufReader.readLine(64 * 1024); // TODO: Define limit somewhere

                // If we see a blank line it's the end of the trailing headers
                if (chunkedLine.length() == 0) {
                    chunkedState = ChunkedState::Eof;
                    return std::make_tuple(0, HttpError::Eof);
                }
                // Not doing any validation of the headers
            }
        }

        chunkedState = ChunkedState::Data;
    }

    // Otherwise, read available data
    uint32_t bytesReadable = (uint32_t)std::min(curLength, (uint64_t)bufferLen);
    uint32_t bytesRead;
    std::tie(bytesRead, err) = bufReader.read(buffer, bytesReadable);

    // On successful read, update bytes available to read and state if no more bytes in this chunk
    if (err == HttpError::Ok) {
        curLength -= bytesRead;
        if (curLength == 0) {
            chunkedState = ChunkedState::Length;
        }
    }

    return std::make_tuple(bytesRead, err);
}

HttpError ChunkedReader::close() {
    char discardBuffer[1024];
    HttpError err;

    // If the user didn't read it, read all chunked content, throwing it away
    while (chunkedState != ChunkedState::Eof) {
        std::tie(std::ignore, err) = read(discardBuffer, sizeof(discardBuffer));
        if (err != HttpError::Ok) {
            return err;
        }
    }
    return HttpError::Ok;
}
