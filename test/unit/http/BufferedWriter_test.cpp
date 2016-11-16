
#include "unit/http/BufferedWriter_test.h"
#include "unit/UnitTest.h"

#include "cupcake/internal/http/BufferedWriter.h"
#include "cupcake/internal/http/StreamSource.h"
#include "cupcake/text/String.h"

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

bool test_bufferedwriter_basic() {
    WriteTestSource testSource;
    BufferedWriter writer;
    writer.init(&testSource, 10);

    HttpError err = writer.write("1234567890", 10);

    if (err != HttpError::Ok) {
        testf("Write inicated failure");
        return false;
    }
    if (testSource.dataWritten.size() != 0) {
        testf("Data written to destination despite buffering");
        return false;
    }

    err = writer.write("a", 1);
    if (err != HttpError::Ok) {
        testf("Write inicated failure");
        return false;
    }
    if (testSource.dataWritten.size() != 11) {
        testf("Wrote %u bytes to destination instead of expected %u", testSource.dataWritten.size(), 11);
        return false;
    }
    if (std::memcmp(&testSource.dataWritten[0], "1234567890a", 11) != 0) {
        testf("Did not write expected data");
        return false;
    }

    return true;
}

bool test_bufferedwriter_flush() {
    WriteTestSource testSource;
    BufferedWriter writer;
    writer.init(&testSource, 10);

    HttpError err = writer.write("12345", 5);

    if (err != HttpError::Ok) {
        testf("Write inicated failure");
        return false;
    }
    if (testSource.dataWritten.size() != 0) {
        testf("Data written to destination despite buffering");
        return false;
    }

    err = writer.flush();
    if (err != HttpError::Ok) {
        testf("Flush indicated failure");
        return false;
    }
    if (testSource.dataWritten.size() != 5) {
        testf("Wrote %u bytes to destination instead of expected %u", testSource.dataWritten.size(), 5);
        return false;
    }
    if (std::memcmp(&testSource.dataWritten[0], "12345", 5) != 0) {
        testf("Did not write expected data");
        return false;
    }

    err = writer.write("1234567890", 10);

    if (err != HttpError::Ok) {
        testf("Write inicated failure");
        return false;
    }
    if (testSource.dataWritten.size() != 5) {
        testf("Data written to destination despite flush clearing buffer");
        return false;
    }

    return true;
}