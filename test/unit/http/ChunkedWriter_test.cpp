
#include "unit/http/ChunkedWriter_test.h"
#include "unit/UnitTest.h"

#include "cupcake/internal/http/ChunkedWriter.h"
#include "cupcake/internal/http/StreamSource.h"
#include "cupcake/internal/text/String.h"

#include <algorithm>
#include <iterator>
#include <vector>

using namespace Cupcake;

class WriteTestSource : public StreamSource {
public:
    WriteTestSource() {}

    std::tuple<StreamSource*, HttpError> accept() override {
        return std::make_tuple(nullptr, HttpError::Ok);
    }
    std::tuple<uint32_t, HttpError> read(char* buffer, uint32_t bufferLen) override {
        return std::make_tuple(0, HttpError::Ok);
    }
    std::tuple<uint32_t, HttpError> readv(INet::IoBuffer* buffers, uint32_t bufferCount) override {
        return std::make_tuple(0, HttpError::Ok);
    }
    HttpError write(const char* buffer, uint32_t bufferLen) override {
        std::copy_n(buffer, bufferLen, std::back_inserter(dataWritten));
        return HttpError::Ok;
    }
    HttpError writev(const INet::IoBuffer* buffers, uint32_t bufferCount) override {
        for (uint32_t i = 0; i < bufferCount; i++) {
            const INet::IoBuffer& bufferIter = buffers[i];
            std::copy_n(bufferIter.buffer, bufferIter.bufferLen, std::back_inserter(dataWritten));
        }
        return HttpError::Ok;
    }
    HttpError close() override {
        return HttpError::Ok;
    }

    std::vector<char> dataWritten;
};

// Tests basic chunked writer functionality
bool test_chunkedwriter_basic() {
    WriteTestSource streamSource;
    ChunkedWriter writer;
    writer.init(&streamSource);

    HttpError err = writer.write("0123456789", 10);
    if (err != HttpError::Ok) {
        testf("Write failed");
        return false;
    }

    err = writer.write("abcd", 4);
    if (err != HttpError::Ok) {
        testf("Write failed");
        return false;
    }

    err = writer.close();
    if (err != HttpError::Ok) {
        testf("Close failed");
        return false;
    }

    const StringRef expected =
        "A\r\n"
        "0123456789\r\n"
        "4\r\n"
        "abcd\r\n"
        "0\r\n"
        "\r\n";

    if (streamSource.dataWritten.size() != expected.length() ||
        std::memcmp(&streamSource.dataWritten[0], expected.data(), expected.length()) != 0) {
        testf("Did not write expected data");
        return false;
    }

    return true;
}

// Tests closing a stream with nothing written
bool test_chunkedwriter_empty() {
    WriteTestSource streamSource;
    ChunkedWriter writer;
    writer.init(&streamSource);

    HttpError err = writer.close();
    if (err != HttpError::Ok) {
        testf("Close failed");
        return false;
    }

    const StringRef expected =
        "0\r\n"
        "\r\n";

    if (streamSource.dataWritten.size() != expected.length() ||
        std::memcmp(&streamSource.dataWritten[0], expected.data(), expected.length()) != 0) {
        testf("Did not write expected data");
        return false;
    }

    return true;
}
