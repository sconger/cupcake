
#ifndef CUPCAKE_STRCONV
#define CUPCAKE_STRCONV

#include "cupcake/text/StringRef.h"

#include <tuple>

namespace Cupcake {

/*
 * String conversion helpers to avoid dealing with the slightly crappy
 * strtol and friends.
 */
namespace Strconv {
    // TODO: These should return -1*required length if they don't fit
    size_t int32ToStr(int32_t value, char* buffer, size_t bufferLen);
    size_t int64ToStr(int64_t value, char* buffer, size_t bufferLen);
    size_t uint32ToStr(uint32_t value, char* buffer, size_t bufferLen);
    size_t uint64ToStr(uint64_t value, char* buffer, size_t bufferLen);

    size_t int32ToStr(int32_t value, uint32_t radix, char* buffer, size_t bufferLen);
    size_t int64ToStr(int64_t value, uint32_t radix, char* buffer, size_t bufferLen);
    size_t uint32ToStr(uint32_t value, uint32_t radix, char* buffer, size_t bufferLen);
    size_t uint64ToStr(uint64_t value, uint32_t radix, char* buffer, size_t bufferLen);

    std::tuple<int32_t, bool> parseInt32(const StringRef str);
    std::tuple<int64_t, bool> parseInt64(const StringRef str);
    std::tuple<uint32_t, bool> parseUint32(const StringRef str);
    std::tuple<uint64_t, bool> parseUint64(const StringRef str);

    std::tuple<int32_t, bool> parseInt32(const StringRef str, uint32_t radix);
    std::tuple<int64_t, bool> parseInt64(const StringRef str, uint32_t radix);
    std::tuple<uint32_t, bool> parseUint32(const StringRef str, uint32_t radix);
    std::tuple<uint64_t, bool> parseUint64(const StringRef str, uint32_t radix);
}

}

#endif // CUPCAKE_STRCONV
