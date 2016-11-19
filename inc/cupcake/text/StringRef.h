
#ifndef CUPCAKE_STRING_REF_H
#define CUPCAKE_STRING_REF_H

#include <cstdint>
#include <functional>

class String;

// A constant reference to text data that makes no copy of the data itself.
//
// This is a helpful way to wrap some to do read-only operations on text data
// without making pointless copies. It can be dangerous if you don't ensure
// the original data stays around.
class StringRef
{
public:
    constexpr StringRef() : strData(nullptr), len(0) {}
    StringRef(const StringRef& other);
    StringRef(const String& str);
    constexpr StringRef(const char* str) : strData(str), len(_strlen(str)) {}
    constexpr StringRef(const char* str, size_t len) : strData(str), len(len) {}
    StringRef& operator=(const StringRef&) = default;
    StringRef(StringRef&& other);
    StringRef& operator=(StringRef&& other);

    const char charAt(size_t pos) const;
    const char operator[](size_t pos) const;

    int32_t compare(const StringRef strRef) const;

    const char* data() const;

    size_t hash() const;

    bool equals(const StringRef strRef) const;

    int32_t engCompareIgnoreCase(const StringRef strRef) const;
    bool engEqualsIgnoreCase(const StringRef strRef) const;

    ptrdiff_t indexOf(char c) const;
    ptrdiff_t indexOf(char c, size_t startIndex) const;
    ptrdiff_t indexOf(const StringRef strRef) const;
    ptrdiff_t indexOf(const StringRef strRef, size_t startIndex) const;

    ptrdiff_t lastIndexOf(char c) const;
    ptrdiff_t lastIndexOf(char c, size_t startIndex) const;
    ptrdiff_t lastIndexOf(const StringRef strRef) const;
    ptrdiff_t lastIndexOf(const StringRef strRef, size_t endIndex) const;

    size_t length() const;

    StringRef substring(size_t startIndex) const;
    StringRef substring(size_t startIndex, size_t endIndex) const;

    bool startsWith(char c) const;
    bool startsWith(const StringRef strRef) const;

    bool endsWith(char c) const;
    bool endsWith(const StringRef strRef) const;

private:
    static constexpr size_t _strlen(const char* str) {
        return (str == nullptr || (*str) == '\0') ? 0 : _strlen(str + 1) + 1;
    }

    const char* strData;
    size_t len;
};

bool operator<(const StringRef strRef1, const StringRef strRef2);
bool operator<(const char* cstr, const StringRef strRef);
bool operator<(const StringRef strRef, const char* cstr);
bool operator<=(const StringRef strRef1, const StringRef strRef2);
bool operator<=(const char* cstr, const StringRef strRef);
bool operator<=(const StringRef strRef, const char* cstr);
bool operator>(const StringRef strRef1, const StringRef strRef2);
bool operator>(const char* cstr, const StringRef strRef);
bool operator>(const StringRef strRef, const char* cstr);
bool operator>=(const StringRef, const StringRef strRef2);
bool operator>=(const char* cstr, const StringRef strRef);
bool operator>=(const StringRef strRef, const char* cstr);
bool operator==(const StringRef strRef1, const StringRef strRef2);
bool operator==(const char* cstr, const StringRef strRef);
bool operator==(const StringRef strRef, const char* cstr);
bool operator!=(const StringRef strRef1, const StringRef strRef2);
bool operator!=(const char* cstr, const StringRef strRef);
bool operator!=(const StringRef strRef, const char* cstr);

// A hash function for std::unordered_map and std::unordered_set
namespace std {
    template <>
    struct hash<StringRef> {
        size_t operator()(const StringRef str) const {
            return str.hash();
        }
    };
}

#endif // CUPCAKE_STRING_REF_H
