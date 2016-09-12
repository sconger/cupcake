
#include "cupcake/text/StringRef.h"

#include <cassert>
#include <cstring>

// Define missing std functions from cstring if visual studio
#ifdef _MSC_VER
#include <memory.h>
namespace std {
void* memrchr(void* ptr, int ch, std::size_t count) {
    unsigned char val = (unsigned char)ch;
    const unsigned char* start = (const unsigned char*)ptr;
    const unsigned char* search = start + count - 1;
    while (search >= start && (*search) != val) {
        search--;
    }
    if (search < start) {
        return nullptr;
    }
    return (void*)search;
}
const void* memrchr(const void* ptr, int ch, std::size_t count) {
    return (const void*)memrchr((void*)ptr, ch, count);
}
}
#endif

// Declare some String members for the incomplete type
class String {
public:
    const char* data() const;
    size_t length() const;
};

StringRef::StringRef() :
    strData(nullptr),
    len(0)
{}

StringRef::StringRef(const StringRef& other) :
    strData(other.strData),
    len(other.len)
{}

StringRef::StringRef(const String& str) :
    strData(str.data()),
    len(str.length())
{}

StringRef::StringRef(const char* str) :
    strData(str),
    len(std::strlen(str))
{}

StringRef::StringRef(const char* str, size_t length) :
    strData(str),
    len(length)
{}

StringRef::StringRef(StringRef&& other) :
	strData(other.strData),
	len(other.len)
{
	other.len = 0;
}

StringRef& StringRef::operator=(StringRef&& other) {
	if (this != &other) {
		strData = other.strData;
		len = other.len;
		other.len = 0;
	}

	return *this;
}

const char StringRef::charAt(size_t pos) const {
    return strData[pos];
}

const char StringRef::operator[](size_t pos) const {
    return strData[pos];
}

int32_t StringRef::compare(const StringRef strRef) const {
    // Emulate behavior of strcmp. That is, do comparison as if this was
    // nul terminated.
    if (len == strRef.len) {
        return std::memcmp(strData, strRef.strData, len);
    } else if (len < strRef.len) {
        int32_t partial = std::memcmp(strData, strRef.strData, len);

        if (partial != 0)
            return partial;
        return 1;
    } else {
        int32_t partial = std::memcmp(strData, strRef.strData, strRef.len);

        if (partial != 0)
            return partial;
        return -1;
    }
}

const char* StringRef::data() const {
    return strData;
}

size_t StringRef::hash() const {
    size_t hash = 0;

    for (size_t i = 0; i < len; i++) {
        size_t c = strData[i];
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

bool StringRef::equals(const StringRef strRef) const {
    if (len != strRef.len)
        return false;

    return std::memcmp(strData, strRef.strData, len) == 0;
}

int32_t StringRef::engCompareIgnoreCase(const StringRef strRef) const {
    for (size_t i = 0;
         i < len && i < strRef.len;
         i++)
    {
        char a = strData[i];

        if (a >= 'a' || a <= 'z') {
            a -= 'a' - 'A';
        }

        char b = strRef.strData[i];

        if (b >= 'a' || b <= 'z') {
            b -= 'a' - 'A';
        }

        if (a != b)
            return a - b;
    }

    if (len < strRef.len) {
        return -1;
    } else if (len > strRef.len) {
        return 1;
    } else {
        return 0;
    }
}

bool StringRef::engEqualsIgnoreCase(const StringRef strRef) const {
    if (len != strRef.len)
        return false;

    for (size_t i = 0; i < len; i++) {
        char a = strData[i];

        if (a >= 'a' || a <= 'z') {
            a -= 'a' - 'A';
        }

        char b = strRef.strData[i];

        if (b >= 'a' || b <= 'z') {
            b -= 'a' - 'A';
        }

        if (a != b) {
            return false;
        }
    }

    return true;
}

ptrdiff_t StringRef::indexOf(char c) const {
    return indexOf(c, 0);
}

ptrdiff_t StringRef::indexOf(char c, size_t startIndex) const {
    const char* start = data();

    const char* match = (const char*)std::memchr(start + startIndex, (int)c, len-startIndex);
    if (match == nullptr) {
        return -1;
    }
    return match - start;
}

ptrdiff_t StringRef::indexOf(const StringRef strRef) const {
    return indexOf(strRef, 0);
}

ptrdiff_t StringRef::indexOf(const StringRef strRef, size_t startIndex) const {
    assert(startIndex >= 0 && startIndex <= len);

    size_t strRefLen = strRef.length();

    // Deal with zero length strings by always reporting a match at index
    // zero. Nothing is always present.
    if (strRefLen == 0) {
        return 0;
    }

    size_t searchLen = len - startIndex;

    // Return -1 for overlong strings as there can be no match.
    if (strRefLen > searchLen) {
        return -1;
    }

    // For short search areas, brute force. It's not worth building a table
    // to speed things up.
    if (searchLen < 256) {
        for (size_t i = startIndex; i < len - strRefLen + 1; i++) {
            bool match = true;

            for (size_t j = 0; match && j < strRefLen; j++) {
                if (strData[i+j] != strRef.strData[j])
                    match = false;
            }

            if (match)
                return i;
        }

        return -1;
    }

    // In the general case, use the Boyer-Moore-Horspool algorithm. This has
    // average case of O(n) and worst case O(n*m), which is worse than the
    // Boyer-Moore algorithm, but has the advantage of not requiring memory
    // allocation.

    const char* strRefData = strRef.strData;
    size_t endIndex = strRefLen - 1;

    // Create an array of "skip" offsets when searching the string
    size_t badCharSkip[256];

    for (uint32_t i = 0; i < 256; i++) {
        badCharSkip[i] = strRefLen;
    }

    for (uint32_t i = 0; i < endIndex; i++) {
        badCharSkip[(unsigned)strRefData[i]] = endIndex - i;
    }

    const char* searchEnd = strData + len - strRefLen;
    const char* searchPtr = strData + startIndex;

    while (searchPtr <= searchEnd) {
        for (size_t scan = endIndex;
             searchPtr[scan] == strRef.strData[scan];
             scan--)
        {
            if (scan == 0) // If complete match
                return searchPtr - strData;
        }

        searchPtr += badCharSkip[(uint8_t)searchPtr[endIndex]];
    }

    // No match found
    return -1;
}

ptrdiff_t StringRef::lastIndexOf(char c) const {
    return lastIndexOf(c, 0);
}

ptrdiff_t StringRef::lastIndexOf(char c, size_t endIndex) const {
    const char* start = data();

    const char* match = (const char*)std::memrchr(start, (int)c, endIndex);
    if (match == nullptr) {
        return -1;
    }
    return match - start;
}

ptrdiff_t StringRef::lastIndexOf(const StringRef strRef) const {
    return lastIndexOf(strRef, len);
}

ptrdiff_t StringRef::lastIndexOf(const StringRef strRef, size_t endIndex) const {
    assert(endIndex >= 0 && endIndex <= len);

    size_t strRefLen = strRef.length();

    // Deal with zero length strings by always reporting a match at index
    // zero. Nothing is always present.
    if (strRefLen == 0) {
        return 0;
    }

    ptrdiff_t searchLen = (ptrdiff_t)endIndex - (ptrdiff_t)strRefLen;

    // Return -1 for overlong strings as there can be no match.
    if (searchLen < 0) {
        return -1;
    }

    // For short search areas, brute force. It's not worth building a table
    // to speed things up.
    if (searchLen < 256) {
        for (ptrdiff_t i = searchLen; i >= 0; i--) {
            bool match = true;

            for (size_t j = 0; match && j < strRefLen; j++) {
                if (strData[i+j] != strRef.strData[j])
                    match = false;
            }

            if (match)
                return i;
        }

        return -1;
    }

    // Like indexOf, using the Boyer-Moore-Horspool algorithm, but reversed.
    // This also changes how the 'skip' indexes are generated.

    const char* strRefData = strRef.strData;
    size_t strRefLast = strRefLen - 1;

    // Create an array of "skip" offsets when searching the string
    size_t skip[255];

    for (uint32_t i = 0; i < 255; i++) {
        skip[i] = strRefLen;
    }

    for (uint32_t i = 0; i < strRefLen; i++) {
        skip[(unsigned)strRefData[i]] = strRefLast - (strRefLast - i);
    }

    // Do the reverse search
    const char* searchStart = strData;
    const char* searchPtr = searchStart + endIndex - strRefLen;

    while (searchPtr >= searchStart) {
        if (std::memcmp(searchPtr, strRefData, strRefLen) == 0) {
            return searchPtr - searchStart;
        }

        searchPtr -= skip[(uint8_t)searchPtr[0]];
    }

    // No match found
    return -1;
}

size_t StringRef::length() const {
    return len;
}

StringRef StringRef::substring(size_t startIndex) const {
    return StringRef(strData+startIndex, len-startIndex);
}

StringRef StringRef::substring(size_t startIndex, size_t endIndex) const {
    return StringRef(strData+startIndex, endIndex - startIndex);
}

bool StringRef::startsWith(char c) const {
    return length() > 0 && charAt(0) == c;
}

bool StringRef::startsWith(const StringRef strRef) const {
    if (len < strRef.len)
        return false;

    return std::memcmp(strData, strRef.strData, strRef.len) == 0;
}

bool StringRef::endsWith(char c) const {
    size_t len = length();
    return len > 0 && charAt(len - 1) == c;
}

bool StringRef::endsWith(const StringRef strRef) const {
    if (len < strRef.len)
        return false;

    return std::memcmp(strData + len - strRef.len, strRef.strData, strRef.len) == 0;
}

bool operator<(const StringRef strRef1, const StringRef strRef2) {
    return strRef1.compare(strRef2) < 0;
}

bool operator<(const char* cstr, const StringRef strRef) {
    return StringRef(cstr).compare(strRef) < 0;
}

bool operator<(const StringRef strRef, const char* cstr) {
    return strRef.compare(StringRef(cstr)) < 0;
}

bool operator<=(const StringRef strRef1, const StringRef strRef2) {
    return strRef1.compare(strRef2) <= 0;
}

bool operator<=(const char* cstr, const StringRef strRef) {
    return StringRef(cstr).compare(strRef) <= 0;
}

bool operator<=(const StringRef strRef, const char* cstr) {
    return strRef.compare(StringRef(cstr)) <= 0;
}

bool operator>(const StringRef strRef1, const StringRef strRef2) {
    return strRef1.compare(strRef2) > 0;
}

bool operator>(const char* cstr, const StringRef strRef) {
    return StringRef(cstr).compare(strRef) > 0;
}

bool operator>(const StringRef strRef, const char* cstr) {
    return strRef.compare(StringRef(cstr)) > 0;
}

bool operator>=(const StringRef strRef1, const StringRef strRef2) {
    return strRef1.compare(strRef2) >= 0;
}

bool operator>=(const char* cstr, const StringRef strRef) {
    return StringRef(cstr).compare(strRef) >= 0;
}

bool operator>=(const StringRef strRef, const char* cstr) {
    return strRef.compare(StringRef(cstr)) >= 0;
}

bool operator==(const StringRef strRef1, const StringRef strRef2) {
    return strRef1.equals(strRef2);
}

bool operator==(const char* cstr, const StringRef strRef) {
    return StringRef(cstr).equals(strRef);
}

bool operator==(const StringRef strRef, const char* cstr) {
    return strRef.equals(StringRef(cstr));
}

bool operator!=(const StringRef strRef1, const StringRef strRef2) {
    return !strRef1.equals(strRef2);
}

bool operator!=(const char* cstr, const StringRef strRef) {
    return !StringRef(cstr).equals(strRef);
}

bool operator!=(const StringRef strRef, const char* cstr) {
    return !strRef.equals(StringRef(cstr));
}
