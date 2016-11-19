
#ifndef CUPCAKE_STRING_H
#define CUPCAKE_STRING_H

#include "cupcake/text/StringRef.h"

#include <functional>

// Usual custom c++ string class. Should avoid exposing. Designed to play nice with StringRef.
class String
{
public:
    String();
    String(const String& str);
    String(String&& str) noexcept;
    String(const StringRef strRef);
    String(const char* cstr);
    String(const char* data, size_t dataLen);

    String& operator=(const String& str);
    String& operator=(String&& str) noexcept;
    String& operator=(const StringRef strRef);
    String& operator=(const char* cstr);

    void append(const StringRef strRef);
    void append(const char* data, size_t dataLen);
    void appendChar(char c);

    const char* c_str() const;
    const char* data() const;

    size_t hash() const;

    const char charAt(size_t pos) const;
    const char operator[](size_t pos) const;

    int32_t compare(const StringRef strRef) const;

    bool equals(const StringRef strRef) const;

    int32_t engCompareIgnoreCase(const StringRef strRef) const;
    bool engEqualsIgnoreCase(const StringRef strRef) const;

    ptrdiff_t indexOf(const StringRef strRef) const;
    ptrdiff_t indexOf(const StringRef strRef, size_t startIndex) const;

    ptrdiff_t lastIndexOf(const StringRef strRef) const;
    ptrdiff_t lastIndexOf(const StringRef strRef, size_t endIndex) const;

    size_t length() const;

    StringRef substring(size_t startIndex) const;
    StringRef substring(size_t startIndex, size_t endIndex) const;

    bool startsWith(const StringRef strRef) const;
    bool endsWith(const StringRef strRef) const;

    void clear();
    void trimToSize(size_t size);
    void reserve(size_t size);

    String& operator+=(const String& str);
    String& operator+=(const StringRef strRef);
    String& operator+=(const char* cstr);
    String& operator+=(char c);

private:
    void append_raw(const char* data, size_t dataLen);
    void append_cstr(const char* cstr, size_t cstrLen);

    inline bool isLong() const;

    inline size_t getShortSize() const;
    inline void setShortSize(size_t size);
    inline size_t getLongCapacity() const;
    inline void setLongCapacity(size_t size);
    inline void setSize(size_t size);

    inline size_t getCapacity() const;
    inline char* getPtr();

    static inline size_t getValidCapacity(size_t size);
    static inline bool isBigEndian();

    enum {minCapacity = (sizeof(size_t) * 2) + sizeof(char*) - 1};

    struct shortForm {
        unsigned char _size;
        char _data[minCapacity];
    };

    struct longForm {
        size_t _capacity;
        size_t _size;
        char*  _data;
    };

    union {
        shortForm _short;
        longForm _long;
    };
};

const String operator+(const String& str1, const String& str2);
const String operator+(const StringRef strRef, const String& str);
const String operator+(const String& str, const StringRef strRef);
const String operator+(const char* cstr, const String& str);
const String operator+(const String& str, const char* cstr);
const String operator+(char c, const String& str);
const String operator+(const String& str, char c);

bool operator<(const String& str, const char* cstr);
bool operator<(const char* cstr, const String& str);
bool operator<=(const String& str, const char* cstr);
bool operator<=(const char* cstr, const String& str);
bool operator>(const String& str, const char* cstr);
bool operator>(const char* cstr, const String& str);
bool operator>=(const String& str, const char* cstr);
bool operator>=(const char* cstr, const String& str);
bool operator==(const char* cstr, const String& str);
bool operator==(const String& str, const char* cstr);
bool operator!=(const char* cstr, const String& strRef);
bool operator!=(const String& str, const char* cstr);

// A hash function for std::unordered_map and std::unordered_set
namespace std {
    template <>
    struct hash<String> {
        size_t operator()(const String& str) const {
            return str.hash();
        }
    };
}

#endif // CUPCAKE_STRING_H
