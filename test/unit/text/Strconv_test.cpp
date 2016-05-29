
#include "unit/text/Strconv_test.h"
#include "unit/UnitTest.h"

#include "cupcake_priv/text/Strconv.h"

#include <limits>
#include <string>
#include <sstream>
#include <vector>

using namespace Cupcake;

static
bool checkString(const char* expected, const char* actual, size_t actualLen) {
    size_t expectedLen = ::strlen(expected);
    if (expectedLen != actualLen) {
        return false;
    }
    return ::memcmp(expected, actual, expectedLen) == 0;
}

template<typename T>
static
std::string asString(T value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

template<typename T>
struct ToStrTestData {
    ToStrTestData(T val, const char* expected) : val(val), expected(expected) {}
    ToStrTestData(T val, std::string expected) : val(val), expected(expected) {}

    T val;
    std::string expected;
};

template<typename T>
struct ParseTestData {
    ParseTestData(const char* val, bool expectSuccess, T expected) : val(val), expectSuccess(expectSuccess), expected(expected) {}
    ParseTestData(std::string val, bool expectSuccess, T expected) : val(val), expectSuccess(expectSuccess), expected(expected) {}

    std::string val;
    bool expectSuccess;
    T expected;
};

bool test_strconv_int32ToStr() {
    char buffer[200];
    size_t len;

    std::vector<ToStrTestData<int32_t>> testDataList{
        ToStrTestData<int32_t>(0, "0"),
        ToStrTestData<int32_t>(123, "123"),
        ToStrTestData<int32_t>(-4567, "-4567"),
        ToStrTestData<int32_t>(std::numeric_limits<int32_t>::max(), asString(std::numeric_limits<int32_t>::max())),
        ToStrTestData<int32_t>(std::numeric_limits<int32_t>::min(), asString(std::numeric_limits<int32_t>::min())),
    };

    for (const ToStrTestData<int32_t>& testData : testDataList) {
        len = Strconv::int32ToStr(testData.val, buffer, sizeof(buffer));
        if (!checkString(testData.expected.c_str(), buffer, len)) {
            testf("Did not get expected value for: %s", testData.expected.c_str());
            return false;
        }
    }
    return true;
}

bool test_strconv_int64ToStr() {
    char buffer[200];
    size_t len;

    std::vector<ToStrTestData<int64_t>> testDataList{
        ToStrTestData<int64_t>(0, "0"),
        ToStrTestData<int64_t>(123, "123"),
        ToStrTestData<int64_t>(-4567, "-4567"),
        ToStrTestData<int64_t>(std::numeric_limits<int64_t>::max(), asString(std::numeric_limits<int64_t>::max())),
        ToStrTestData<int64_t>(std::numeric_limits<int64_t>::min(), asString(std::numeric_limits<int64_t>::min())),
    };

    for (const ToStrTestData<int64_t>& testData : testDataList) {
        len = Strconv::int64ToStr(testData.val, buffer, sizeof(buffer));
        if (!checkString(testData.expected.c_str(), buffer, len)) {
            testf("Did not get expected value for: %s", testData.expected.c_str());
            return false;
        }
    }
    return true;
}

bool test_strconv_uint32ToStr() {
    char buffer[200];
    size_t len;

    std::vector<ToStrTestData<uint32_t>> testDataList{
        ToStrTestData<uint32_t>(0, "0"),
        ToStrTestData<uint32_t>(123, "123"),
        ToStrTestData<uint32_t>(1111111, "1111111"),
        ToStrTestData<uint32_t>(std::numeric_limits<uint32_t>::max(), asString(std::numeric_limits<uint32_t>::max())),
    };

    for (const ToStrTestData<uint32_t>& testData : testDataList) {
        len = Strconv::uint32ToStr(testData.val, buffer, sizeof(buffer));
        if (!checkString(testData.expected.c_str(), buffer, len)) {
            testf("Did not get expected value for: %s", testData.expected.c_str());
            return false;
        }
    }
    return true;
}

bool test_strconv_uint64ToStr() {
    char buffer[200];
    size_t len;

    std::vector<ToStrTestData<uint64_t>> testDataList{
        ToStrTestData<uint64_t>(0, "0"),
        ToStrTestData<uint64_t>(123, "123"),
        ToStrTestData<uint64_t>(1111111, "1111111"),
        ToStrTestData<uint64_t>(std::numeric_limits<uint64_t>::max(), asString(std::numeric_limits<uint64_t>::max())),
    };

    for (const ToStrTestData<uint64_t>& testData : testDataList) {
        len = Strconv::uint64ToStr(testData.val, buffer, sizeof(buffer));
        if (!checkString(testData.expected.c_str(), buffer, len)) {
            testf("Did not get expected value for: %s", testData.expected.c_str());
            return false;
        }
    }
    return true;
}

bool test_strconv_parseInt32() {
    std::vector<ParseTestData<int32_t>> testDataList{
        ParseTestData<int32_t>("0", true, 0),
        ParseTestData<int32_t>("00", true, 0),
        ParseTestData<int32_t>("1234", true, 1234),
        ParseTestData<int32_t>("-5678", true, -5678),
        ParseTestData<int32_t>(asString(std::numeric_limits<int32_t>::max()), true, std::numeric_limits<int32_t>::max()),
        ParseTestData<int32_t>(asString(std::numeric_limits<int32_t>::min()), true, std::numeric_limits<int32_t>::min()),
        ParseTestData<int32_t>("", false, 0),
        ParseTestData<int32_t>("-", false, 0),
        ParseTestData<int32_t>("a", false, 0),
        ParseTestData<int32_t>(" 1", false, 0),
        ParseTestData<int32_t>("1 ", false, 0),
        ParseTestData<int32_t>("1.0", false, 0),
    };

    for (const ParseTestData<int32_t>& testData : testDataList) {
        int32_t resVal;
        bool parsed;
        std::tie(resVal, parsed) = Strconv::parseInt32(StringRef(testData.val.c_str()));
        
        if (parsed != testData.expectSuccess) {
            if (testData.expectSuccess) {
                testf("Did not expect \"%s\" to parse, but success indicated", testData.val.c_str());
            } else {
                testf("Expected \"%s\" to parse, but failure indicated", testData.val.c_str());
            }
            return false;
        }

        if (testData.expectSuccess &&
            resVal != testData.expected) {
            testf("Did not get expected value for \"%s\"", testData.val.c_str());
            return false;
        }
    }
    return true;
}

bool test_strconv_parseInt64() {
    std::vector<ParseTestData<int64_t>> testDataList{
        ParseTestData<int64_t>("0", true, 0),
        ParseTestData<int64_t>("00", true, 0),
        ParseTestData<int64_t>("1234", true, 1234),
        ParseTestData<int64_t>("-5678", true, -5678),
        ParseTestData<int64_t>(asString(std::numeric_limits<int64_t>::max()), true, std::numeric_limits<int64_t>::max()),
        ParseTestData<int64_t>(asString(std::numeric_limits<int64_t>::min()), true, std::numeric_limits<int64_t>::min()),
        ParseTestData<int64_t>("", false, 0),
        ParseTestData<int64_t>("-", false, 0),
        ParseTestData<int64_t>("a", false, 0),
        ParseTestData<int64_t>(" 1", false, 0),
        ParseTestData<int64_t>("1 ", false, 0),
        ParseTestData<int64_t>("1.0", false, 0),
    };

    for (const ParseTestData<int64_t>& testData : testDataList) {
        int64_t resVal;
        bool parsed;
        std::tie(resVal, parsed) = Strconv::parseInt64(StringRef(testData.val.c_str()));

        if (parsed != testData.expectSuccess) {
            if (testData.expectSuccess) {
                testf("Did not expect \"%s\" to parse, but success indicated", testData.val.c_str());
            } else {
                testf("Expected \"%s\" to parse, but failure indicated", testData.val.c_str());
            }
            return false;
        }

        if (testData.expectSuccess &&
            resVal != testData.expected) {
            testf("Did not get expected value for \"%s\"", testData.val.c_str());
            return false;
        }
    }
    return true;
}

bool test_strconv_parseUint32() {
    std::vector<ParseTestData<uint32_t>> testDataList{
        ParseTestData<uint32_t>("0", true, 0),
        ParseTestData<uint32_t>("00", true, 0),
        ParseTestData<uint32_t>("1234", true, 1234),
        ParseTestData<uint32_t>(asString(std::numeric_limits<uint32_t>::max()), true, std::numeric_limits<uint32_t>::max()),
        ParseTestData<uint32_t>(asString(std::numeric_limits<uint32_t>::min()), true, std::numeric_limits<uint32_t>::min()),
        ParseTestData<uint32_t>("-5678", false, 0),
        ParseTestData<uint32_t>("", false, 0),
        ParseTestData<uint32_t>("-", false, 0),
        ParseTestData<uint32_t>("a", false, 0),
        ParseTestData<uint32_t>(" 1", false, 0),
        ParseTestData<uint32_t>("1 ", false, 0),
        ParseTestData<uint32_t>("1.0", false, 0),
    };

    for (const ParseTestData<uint32_t>& testData : testDataList) {
        uint32_t resVal;
        bool parsed;
        std::tie(resVal, parsed) = Strconv::parseUint32(StringRef(testData.val.c_str()));

        if (parsed != testData.expectSuccess) {
            if (testData.expectSuccess) {
                testf("Did not expect \"%s\" to parse, but success indicated", testData.val.c_str());
            } else {
                testf("Expected \"%s\" to parse, but failure indicated", testData.val.c_str());
            }
            return false;
        }

        if (testData.expectSuccess &&
            resVal != testData.expected) {
            testf("Did not get expected value for \"%s\"", testData.val.c_str());
            return false;
        }
    }
    return true;
}

bool test_strconv_parseUint64() {
    std::vector<ParseTestData<uint64_t>> testDataList{
        ParseTestData<uint64_t>("0", true, 0),
        ParseTestData<uint64_t>("00", true, 0),
        ParseTestData<uint64_t>("1234", true, 1234),
        ParseTestData<uint64_t>(asString(std::numeric_limits<uint64_t>::max()), true, std::numeric_limits<uint64_t>::max()),
        ParseTestData<uint64_t>(asString(std::numeric_limits<uint64_t>::min()), true, std::numeric_limits<uint64_t>::min()),
        ParseTestData<uint64_t>("-5678", false, 0),
        ParseTestData<uint64_t>("", false, 0),
        ParseTestData<uint64_t>("-", false, 0),
        ParseTestData<uint64_t>("a", false, 0),
        ParseTestData<uint64_t>(" 1", false, 0),
        ParseTestData<uint64_t>("1 ", false, 0),
        ParseTestData<uint64_t>("1.0", false, 0),
    };

    for (const ParseTestData<uint64_t>& testData : testDataList) {
        uint64_t resVal;
        bool parsed;
        std::tie(resVal, parsed) = Strconv::parseUint64(StringRef(testData.val.c_str()));

        if (parsed != testData.expectSuccess) {
            if (testData.expectSuccess) {
                testf("Did not expect \"%s\" to parse, but success indicated", testData.val.c_str());
            } else {
                testf("Expected \"%s\" to parse, but failure indicated", testData.val.c_str());
            }
            return false;
        }

        if (testData.expectSuccess &&
            resVal != testData.expected) {
            testf("Did not get expected value for \"%s\"", testData.val.c_str());
            return false;
        }
    }
    return true;
}
