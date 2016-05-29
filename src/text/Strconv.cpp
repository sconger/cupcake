// strconf.cpp

#include "cupcake_priv/text/Strconv.h"

#include <algorithm>

static
int8_t charValue[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 00-0F
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 10-1F
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 20-2F
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, // 30-3F
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 40-4F
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 50-5F
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 60-6F
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 70-7F
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 80-8F
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 90-9F
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // A0-AF
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // B0-BF
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // C0-CF
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // D0-DF
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // E0-EF
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  // F0-FF
};

static
const char two_digit_lookup[201] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

#define P01 10
#define P02 100
#define P03 1000
#define P04 10000
#define P05 100000
#define P06 1000000
#define P07 10000000
#define P08 100000000
#define P09 1000000000
#define P10 10000000000ULL
#define P11 100000000000ULL
#define P12 1000000000000ULL

#define INT32_MIN_STR "-2147483648"
#define INT64_MIN_STR "-9223372036854775808"

#define INT32_MAX_LEN 11  // -2147483648
#define INT64_MAX_LEN 20  // -9223372036854775808
#define UINT32_MAX_LEN 10 // 4294967295
#define UINT64_MAX_LEN 20 // 18446744073709551615


// Integer to string helper functions
// See: https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920
static
size_t digitsBase10(uint32_t v) {
    if (v < P01) return 1;
    if (v < P02) return 2;
    if (v < P03) return 3;
    if (v < P06) {
        if (v < P04) {
            return 4;
        }
        return 5 + (v >= P05);
    } else if (v < P09) {
        if (v < P07) {
            return 7;
        }
        return 8 + (v >= P08);
    }
    return 10;
}

static
size_t digitsBase10(uint64_t v) {
    if (v < P01) return 1;
    if (v < P02) return 2;
    if (v < P03) return 3;
    if (v < P12) {
        if (v < P08) {
            if (v < P06) {
                if (v < P04) {
                    return 4;
                }
                return 5 + (v >= P05);
            }
            return 7 + (v >= P07);
        }
        if (v < P10) {
            return 9 + (v >= P09);
        }
        return 11 + (v >= P11);
    }
    return 12 + digitsBase10(v / P12);
}

static
size_t uint32ToStr(uint32_t value, char* buffer) {
    size_t length = digitsBase10(value);
    size_t next = length - 1;
    while (value >= 100) {
        auto const i = (value % 100) * 2;
        value /= 100;
        buffer[next] = two_digit_lookup[i + 1];
        buffer[next - 1] = two_digit_lookup[i];
        next -= 2;
    }
    // Handle last 1-2 digits
    if (value < 10) {
        buffer[next] = '0' + char(value);
    } else {
        auto i = uint32_t(value) * 2;
        buffer[next] = two_digit_lookup[i + 1];
        buffer[next - 1] = two_digit_lookup[i];
    }
    return length;
}

static
size_t uint64ToStr(uint64_t value, char* buffer) {
    size_t length = digitsBase10(value);
    size_t next = length - 1;
    while (value >= 100) {
        auto const i = (value % 100) * 2;
        value /= 100;
        buffer[next] = two_digit_lookup[i + 1];
        buffer[next - 1] = two_digit_lookup[i];
        next -= 2;
    }
    // Handle last 1-2 digits
    if (value < 10) {
        buffer[next] = '0' + char(value);
    } else {
        auto i = uint32_t(value) * 2;
        buffer[next] = two_digit_lookup[i + 1];
        buffer[next - 1] = two_digit_lookup[i];
    }
    return length;
}

static
size_t int32ToStr(int32_t value, char* buffer) {
    if (value == INT32_MIN) {
        size_t len = sizeof(INT32_MIN_STR) - 1;
        memcpy(buffer, INT32_MIN_STR, len);
        return len;
    } else if (value >= 0) {
        return uint32ToStr((uint32_t)value, buffer);
    } else {
        buffer[0] = '-';
        return uint32ToStr((uint32_t)(value * -1), buffer + 1) + 1;
    }
}

static
size_t int64ToStr(int64_t value, char* buffer) {
    if (value == INT64_MIN) {
        size_t len = sizeof(INT64_MIN_STR) - 1;
        memcpy(buffer, INT64_MIN_STR, len);
        return len;
    } else if (value >= 0) {
        return uint64ToStr((uint64_t)value, buffer);
    } else {
        buffer[0] = '-';
        return uint64ToStr((uint64_t)(value * -1), buffer + 1) + 1;
    }
}

namespace Cupcake {

namespace Strconv {

size_t int32ToStr(int32_t value, char* buffer, size_t bufferLen) {
    char tempBuf[INT32_MAX_LEN];
    size_t size = ::int32ToStr(value, tempBuf);
    ::memcpy(buffer, tempBuf, std::min(size, bufferLen));
    return size;
}

size_t int64ToStr(int64_t value, char* buffer, size_t bufferLen) {
    char tempBuf[INT64_MAX_LEN];
    size_t size = ::int64ToStr(value, tempBuf);
    ::memcpy(buffer, tempBuf, std::min(size, bufferLen));
    return size;
}

size_t uint32ToStr(uint32_t value, char* buffer, size_t bufferLen) {
    char tempBuf[UINT32_MAX_LEN];
    size_t size = ::uint32ToStr(value, tempBuf);
    ::memcpy(buffer, tempBuf, std::min(size, bufferLen));
    return size;
}

size_t uint64ToStr(uint64_t value, char* buffer, size_t bufferLen) {
    char tempBuf[UINT64_MAX_LEN];
    size_t size = ::uint64ToStr(value, tempBuf);
    ::memcpy(buffer, tempBuf, std::min(size, bufferLen));
    return size;
}

std::tuple<int32_t, bool> parseInt32(const StringRef str, uint32_t radix) {
    if (radix < 2 || radix > 16) {
        return std::make_tuple(0, false);
    }

    const char* data = str.data();
    size_t strLen = str.length();

    int32_t result = 0;
    bool negative = false;
    uint32_t i = 0;

    int32_t limit;
    int32_t multmin;

    // Check for empty string
    if (strLen == 0) {
        return std::make_tuple(0, false);
    }

    // If there is a negative sign, note it and the appropriate limit value
    if (data[0] == '-') {
        if (strLen == 1) {
            return std::make_tuple(0, false);
        }

        negative = true;
        limit = INT32_MIN;
        i++;
    } else {
        limit = -INT32_MAX;
    }

    // Generate a guard value which we can use to detect "too many digits"
    multmin = limit / (int32_t)radix;

    // Parse the first digit
    if (i < strLen) {
        int8_t digitVal = charValue[(uint8_t)data[i]];
        if (digitVal < 0 || (uint32_t)digitVal >= radix) {
            return std::make_tuple(0, false);
        } else {
            result = -digitVal;
        }
        i++;
    }

    // Loop, parsing the remaining digits
    while (i < strLen) {
        // Accumulating negatively avoids issues near INT32_MAX
        int8_t digitVal = charValue[(uint8_t)data[i]];
        if (digitVal < 0 || (uint32_t)digitVal >= radix) {
            return std::make_tuple(0, false);
        }

        // Check if this will produce too many digits
        if (result < multmin) {
            return std::make_tuple(0, false);
        }

        result *= radix;

        // Check if this will overflow the maximum value
        if (result < limit + digitVal) {
            return std::make_tuple(0, false);
        }
        result -= digitVal;
        i++;
    }

    if (!negative) {
        result = -result;
    }
    return std::make_tuple(result, true);
}

std::tuple<int64_t, bool> parseInt64(const StringRef str, uint32_t radix) {
    if (radix < 2 || radix > 16) {
        return std::make_tuple(0, false);
    }

    const char* data = str.data();
    size_t strLen = str.length();

    int64_t result = 0;
    bool negative = false;
    uint32_t i = 0;

    int64_t limit;
    int64_t multmin;

    // Check for empty string
    if (strLen == 0) {
        return std::make_tuple(0, false);
    }

    // If there is a negative sign, note it and the appropriate limit value
    if (data[0] == '-') {
        if (strLen == 1) {
            return std::make_tuple(0, false);
        }

        negative = true;
        limit = INT64_MIN;
        i++;
    } else {
        limit = -INT64_MAX;
    }

    // Generate a guard value which we can use to detect "too many digits"
    multmin = limit / (int64_t)radix;

    // Parse the first digit
    if (i < strLen) {
        int8_t digitVal = charValue[(uint8_t)data[i]];
        if (digitVal < 0 || (uint32_t)digitVal >= radix) {
            return std::make_tuple(0, false);
        } else {
            result = -digitVal;
        }
        i++;
    }

    // Loop, parsing the remaining digits
    while (i < strLen) {
        // Accumulating negatively avoids issues near INT32_MAX
        int8_t digitVal = charValue[(uint8_t)data[i]];
        if (digitVal < 0 || (uint32_t)digitVal >= radix) {
            return std::make_tuple(0, false);
        }

        // Check if this will produce too many digits
        if (result < multmin) {
            return std::make_tuple(0, false);
        }

        result *= radix;

        // Check if this will overflow the maximum value
        if (result < limit + digitVal) {
            return std::make_tuple(0, false);
        }
        result -= digitVal;
        i++;
    }

    if (!negative) {
        result = -result;
    }
    return std::make_tuple(result, true);
}

std::tuple<uint32_t, bool> parseUint32(const StringRef str, uint32_t radix) {
    if (radix < 2 || radix > 16) {
        return std::make_tuple(0, false);
    }

    size_t strLen = str.length();
    const char* data = str.data();

    uint32_t result = 0;
    uint32_t i = 0;

    uint32_t multmin;

    // Check for empty string
    if (strLen == 0) {
        return std::make_tuple(0, false);
    }

    // Generate a guard value which we can use to detect "too many digits"
    multmin = UINT32_MAX / radix;

    // Parse the first digit
    if (i < strLen) {
        int8_t digitVal = charValue[(uint8_t)data[i]];
        if (digitVal < 0 || (uint32_t)digitVal >= radix) {
            return std::make_tuple(0, false);
        }

        result = digitVal;
        i++;
    }

    // Loop, parsing the remaining digits
    while (i < strLen) {
        int8_t digitVal = charValue[(uint8_t)data[i]];
        if (digitVal < 0 || (uint32_t)digitVal >= radix) {
            return std::make_tuple(0, false);
        }

        // Check if this will produce too many digits
        if (result > multmin) {
            return std::make_tuple(0, false);
        }

        result *= radix;

        // Check if this will overflow the maximum value
        if (result > UINT32_MAX - digitVal) {
            return std::make_tuple(0, false);
        }
        result += digitVal;
        i++;
    }

    return std::make_tuple(result, true);
}

std::tuple<uint64_t, bool> parseUint64(const StringRef str, uint32_t radix) {
    if (radix < 2 || radix > 16) {
        return std::make_tuple(0, false);
    }

    size_t strLen = str.length();
    const char* data = str.data();

    uint64_t result = 0;
    uint32_t i = 0;

    uint64_t multmin;

    // Check for empty string
    if (strLen == 0) {
        return std::make_tuple(0, false);
    }

    // Generate a guard value which we can use to detect "too many digits"
    multmin = UINT64_MAX / (uint64_t)radix;

    // Parse the first digit
    if (i < strLen) {
        int8_t digitVal = charValue[(uint8_t)data[i]];
        if (digitVal < 0 || (uint32_t)digitVal >= radix) {
            return std::make_tuple(0, false);
        }

        result = digitVal;
        i++;
    }

    // Loop, parsing the remaining digits
    while (i < strLen) {
        int8_t digitVal = charValue[(uint8_t)data[i]];
        if (digitVal < 0 || (uint32_t)digitVal >= radix) {
            return std::make_tuple(0, false);
        }

        // Check if this will produce too many digits
        if (result > multmin) {
            return std::make_tuple(0, false);
        }

        result *= radix;

        // Check if this will overflow the maximum value
        if (result > UINT64_MAX - digitVal) {
            return std::make_tuple(0, false);
        }
        result += digitVal;
        i++;
    }

    return std::make_tuple(result, true);
}

std::tuple<int32_t, bool> parseInt32(const StringRef str) {
    return parseInt32(str, 10);
}

std::tuple<int64_t, bool> parseInt64(const StringRef str) {
    return parseInt64(str, 10);
}

std::tuple<uint32_t, bool> parseUint32(const StringRef str) {
    return parseUint32(str, 10);
}

std::tuple<uint64_t, bool> parseUint64(const StringRef str) {
    return parseUint64(str, 10);
}

} // End namespace Strconv

} // End namespace Cupcake
