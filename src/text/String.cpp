
#include "cupcake/text/String.h"

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <memory>


String::String() {
    _short._size = 0;
    _short._data[0] = '\0';
}

String::String(const String& str) {
    size_t len = str.length();

    if (len < minCapacity) {
        _short = str._short;
    } else {
        size_t newCap = getValidCapacity(len+1);
        char* data = new char[newCap];
        const char* otherData = str._long._data;
        setLongCapacity(newCap);
        _long._size = len;
        _long._data = data;
        std::memcpy(data, otherData, newCap);
    }
}

String::String(String&& str) noexcept {
    _long = str._long; // Default copy

    str._short._size = 0;
    str._short._data[0] = '\0';
}

String::String(const StringRef strRef) {
    size_t len = strRef.length();
    char* data;

    if (len < minCapacity) {
        setShortSize(len);
        data = _short._data;
    } else {
        size_t newCap = getValidCapacity(len+1);
        data = new char[newCap];
        setLongCapacity(newCap);
        _long._size = len;
        _long._data = data;
    }

    std::memcpy(data, strRef.data(), len);
    data[len] = '\0';
}

String::String(const char* cstr) {
    size_t len = std::strlen(cstr);
    size_t lenWithNull = len+1;
    char* data;

    if (len < minCapacity) {
        setShortSize(len);
        data = _short._data;
    } else {
        size_t newCap = getValidCapacity(lenWithNull);
        data = new char[newCap];
        setLongCapacity(newCap);
        _long._size = len;
        _long._data = data;
    }

    std::memcpy(data, cstr, lenWithNull);
}

String::String(const char* cstr, size_t len) {
    char* data;

    if (len < minCapacity) {
        setShortSize(len);
        data = _short._data;
    } else {
        size_t newCap = getValidCapacity(len+1);
        data = new char[newCap];
        setLongCapacity(newCap);
        _long._size = len;
        _long._data = data;
    }

    std::memcpy(data, cstr, len);
    data[len] = '\0';
}

String& String::operator=(const String& str) {
    if (this == &str) {
        return *this;
    }

    if (isLong()) {
        delete[] _long._data;
    }

    if (str.isLong()) {
        size_t len = str.length();
        size_t lenWithNull = len+1;
        size_t newCap = getValidCapacity(lenWithNull);

        setLongCapacity(newCap);
        _long._size = len;
        _long._data = new char[newCap];
        std::memcpy(_long._data, str._long._data, lenWithNull);
    } else {
        _short = str._short;
    }

    return *this;
}

String& String::operator=(String&& str) noexcept {
    if (this == &str) {
        return *this;
    }

    _long = str._long; // Default copy

    str._short._size = 0;
    str._short._data[0] = '\0';

    return *this;
}

String& String::operator=(const StringRef strRef) {
    // Because it's possible to assign a StringRef that is a substring of
    // this String, we have to use memmove instead of memcpy and defer
    // deletion of the old data until the end.
    const char* oldData = nullptr;

    if (isLong()) {
        oldData = _long._data;
    }

    size_t len = strRef.length();
    size_t lenWithNull = len+1;
    char* data;

    if (len < minCapacity) {
        setShortSize(len);
        data = _short._data;
    } else {
        size_t newCap = getValidCapacity(lenWithNull);
        data = new char[newCap];
        setLongCapacity(newCap);
        _long._size = len;
        _long._data = data;
    }

    std::memmove(data, strRef.data(), len);
    data[len] = '\0';

    delete[] oldData;

    return *this;
}

String& String::operator=(const char* cstr) {
    if (isLong()) {
        delete[] _long._data;
    }

    size_t len = std::strlen(cstr);
    size_t lenWithNull = len+1;
    char* data;

    if (len < minCapacity) {
        setShortSize(len);
        data = _short._data;
    } else {
        size_t newCap = getValidCapacity(lenWithNull);
        data = new char[newCap];
        setLongCapacity(newCap);
        _long._size = len;
        _long._data = data;
    }

    std::memcpy(data, cstr, lenWithNull);

    return *this;
}

void String::append(const StringRef strRef) {
    append_raw(strRef.data(), strRef.length());
}

void String::append(const char* data, size_t dataLen) {
    append_raw(data, dataLen);
}

void String::appendChar(char c) {
    append_raw(&c, 1);
}

void String::append_raw(const char* appendData, size_t dataLen)
{
    size_t len = length();
    size_t capacity = getCapacity();
    char* data = getPtr();

    if (len + dataLen < capacity) {
        std::memcpy(data+len, appendData, dataLen);
        data[len+dataLen] = '\0';
        setSize(len+dataLen);
    } else {
        size_t newLen = len + dataLen;
        size_t newCapacity = getValidCapacity(newLen * 2);
        char* newData = new char[newCapacity];
        std::memcpy(newData, data, len);

        if (isLong()) {
            delete[] _long._data;
        }

        std::memcpy(newData + len, appendData, dataLen);
        newData[len + dataLen] = '\0';

        setLongCapacity(newCapacity);
        _long._size = newLen;
        _long._data = newData;
    }
}

void String::append_cstr(const char* cstr, size_t cstrLen)
{
    size_t len = length();
    size_t capacity = getCapacity();
    char* data = getPtr();

    if (len + cstrLen < capacity) {
        std::memcpy(data+len, cstr, cstrLen+1);
        setSize(len+cstrLen);
    } else {
        size_t newLen = len + cstrLen + 1;
        size_t newCapacity = getValidCapacity(newLen * 2);
        char* newData = new char[newCapacity];
        std::memcpy(newData, data, len);

        if (isLong()) {
            delete[] _long._data;
        }

        std::memcpy(newData + len, cstr, cstrLen+1);

        setLongCapacity(newCapacity);
        _long._size = newLen;
        _long._data = newData;
    }
}

const char* String::c_str() const {
    return data();
}

const char* String::data() const {
    return isLong() ? _long._data : _short._data;
}

size_t String::hash() const {
    return StringRef(*this).hash();
}

const char String::charAt(size_t pos) const {
    return data()[pos];
}

const char String::operator[](size_t pos) const {
    return data()[pos];
}

int32_t String::compare(const StringRef strRef) const {
    size_t len = length();
    size_t strRefLen = strRef.length();
    const char* localData = data();
    const char* strRefData = strRef.data();

    // Emulate behavior of strcmp. That is, do comparison as if the StringRef
    // was null terminated.
    if (len == strRefLen) {
        return std::memcmp(localData, strRefData, len);
    } else if (len < strRefLen) {
        int32_t partial = std::memcmp(localData, strRefData, len);

        if (partial != 0)
            return partial;
        return 1;
    } else {
        int32_t partial = std::memcmp(localData, strRefData, strRefLen);

        if (partial != 0)
            return partial;
        return -1;
    }
}

bool String::equals(const StringRef strRef) const {
    size_t len = length();
    if (len != strRef.length()) {
        return false;
    }

    return std::memcmp(data(), strRef.data(), len) == 0;
}

int32_t String::engCompareIgnoreCase(const StringRef strRef) const {
    return StringRef(*this).engCompareIgnoreCase(strRef);
}

bool String::engEqualsIgnoreCase(const StringRef strRef) const {
    return StringRef(*this).engEqualsIgnoreCase(strRef);
}

ptrdiff_t String::indexOf(const StringRef strRef) const {
    return indexOf(strRef, 0);
}

ptrdiff_t String::indexOf(const StringRef strRef, size_t startIndex) const {
    return StringRef(*this).indexOf(strRef, startIndex);
}

ptrdiff_t String::lastIndexOf(const StringRef strRef) const {
    return lastIndexOf(strRef, length());
}

ptrdiff_t String::lastIndexOf(const StringRef strRef, size_t endIndex) const {
    return StringRef(*this).lastIndexOf(strRef, endIndex);
}

size_t String::length() const {
    return isLong() ? _long._size : getShortSize();
}

StringRef String::substring(size_t startIndex) const {
    return StringRef(data()+startIndex, length()-startIndex);
}

StringRef String::substring(size_t startIndex, size_t endIndex) const {
    return StringRef(data()+startIndex, endIndex-startIndex);
}

bool String::startsWith(const StringRef strRef) const {
    return StringRef(*this).startsWith(strRef);
}

bool String::endsWith(const StringRef strRef) const {
    return StringRef(*this).endsWith(strRef);
}

void String::clear() {
    setSize(0);
    getPtr()[0] = '\0';
}

void String::trimToSize(size_t size) {
    if (size > length()) {
        return;
    }

    setSize(size);
    getPtr()[size] = '\0';
}

void String::reserve(size_t size) {
    // Do nothing if already reserved
    if (size <= getCapacity()) {
        return;
    }

    size_t oldLength = length();
    size_t newCapacity = getValidCapacity(size+1);
    char* newBuffer = new char[newCapacity];

    size_t len = length();
    std::memcpy(newBuffer, data(), len+1); // Always at least a null

    if (isLong()) {
        delete[] _long._data;
    }

    setLongCapacity(newCapacity);
    _long._size = oldLength;
    _long._data = newBuffer;
}

void String::trim() {
    // TODO
}

bool String::isLong() const {
    if (isBigEndian()) {
        return (_short._size & 0x80) != 0;
    } else {
        return (_short._size & 0x1) != 0;
    }
}

size_t String::getShortSize() const {
    if (isBigEndian()) {
        return _short._size;
    } else {
        return _short._size >> 1;
    }
}

void String::setShortSize(size_t size) {
    if (isBigEndian()) {
        _short._size = (unsigned char)(size);
    } else {
        _short._size = (unsigned char)(size << 1);
    }
}

size_t String::getLongCapacity() const {
    if (isBigEndian()) {
        size_t capUnMask = size_t(~0) >> 1;
        return _long._capacity & capUnMask;
    } else {
        size_t capUnMask = ~(0x1ul);
        return _long._capacity & capUnMask;
    }
}

void String::setLongCapacity(size_t size) {
    if (isBigEndian()) {
        size_t capMask = ~(size_t(~0) >> 1);
        _long._capacity = capMask | size;
    } else {
        size_t capMask = 0x1ul;
        _long._capacity = capMask | size;
    }
}

void String::setSize(size_t size) {
    if (isLong()) {
        _long._size = size;
    } else {
        setShortSize(size);
    }
}

size_t String::getCapacity() const {
    return isLong() ? getLongCapacity() : minCapacity;
}

char* String::getPtr() {
    return isLong() ? _long._data : _short._data;
}

// We need to avoid having capacities with the low bit set. Rounding up to the
// nearest 8.
size_t String::getValidCapacity(size_t size) {
    size_t mask = size & 0x7;
    if (mask) {
        return size + (8 - mask);
    }
    return size;
}

// Obviously, this could be a macro, but there is no consistent one to use
bool String::isBigEndian() {
    size_t one = 1;
    char* highBit = (char*)&one;
    return highBit == 0;
}

String& String::operator+=(const String& str) {
    append(str);
    return *this;
}

String& String::operator+=(const StringRef strRef) {
    append(strRef);
    return *this;
}

String& String::operator+=(const char* cstr) {
    append(cstr);
    return *this;
}

String& String::operator+=(char c) {
    appendChar(c);
    return *this;
}

const String operator+(const String& str1, const String& str2) {
    String ret;
    ret.reserve(str1.length() + str2.length());
    ret.append(str1);
    ret.append(str2);
    return ret;
}

const String operator+(const StringRef strRef, const String& str) {
    String ret;
    ret.reserve(strRef.length() + str.length());
    ret.append(strRef);
    ret.append(str);
    return ret;
}

const String operator+(const String& str, const StringRef strRef) {
    String ret;
    ret.reserve(str.length() + strRef.length());
    ret.append(str);
    ret.append(strRef);
    return ret;
}

const String operator+(const char* cstr, const String& str) {
    String ret;
    size_t cstrLen = std::strlen(cstr);
    ret.reserve(cstrLen + str.length());
    ret.append(cstr, cstrLen);
    ret.append(str);
    return ret;
}

const String operator+(const String& str, const char* cstr) {
    String ret;
    size_t cstrLen = std::strlen(cstr);
    ret.reserve(str.length() + cstrLen);
    ret.append(str);
    ret.append(cstr, cstrLen);
    return ret;
}

const String operator+(char c, const String& str) {
    String ret;
    ret.reserve(str.length() + 1);
    ret.appendChar(c);
    ret.append(str);
    return ret;
}

const String operator+(const String& str, char c) {
    String ret;
    ret.reserve(str.length() + 1);
    ret.append(str);
    ret.appendChar(c);
    return ret;
}

bool operator<(const String& str, const char* cstr) {
    return std::strcmp(str.data(), cstr) < 0;
}

bool operator<(const char* cstr, const String& str) {
    return std::strcmp(cstr, str.data()) < 0;
}

bool operator<=(const String& str, const char* cstr) {
    return std::strcmp(str.data(), cstr) <= 0;
}

bool operator<=(const char* cstr, const String& str) {
    return std::strcmp(cstr, str.data()) <= 0;
}

bool operator>(const String& str, const char* cstr) {
    return std::strcmp(str.data(), cstr) > 0;
}

bool operator>(const char* cstr, const String& str) {
    return std::strcmp(cstr, str.data()) > 0;
}

bool operator>=(const String& str, const char* cstr) {
    return std::strcmp(str.data(), cstr) >= 0;
}

bool operator>=(const char* cstr, const String& str) {
    return std::strcmp(cstr, str.data()) >= 0;
}

bool operator==(const char* cstr, const String& str) {
    return str.compare(StringRef(cstr)) == 0;
}

bool operator==(const String& str, const char* cstr) {
    return str.compare(StringRef(cstr)) == 0;
}

bool operator!=(const char* cstr, const String& str) {
    return str.compare(StringRef(cstr)) != 0;
}

bool operator!=(const String& str, const char* cstr) {
    return str.compare(StringRef(cstr)) != 0;
}
