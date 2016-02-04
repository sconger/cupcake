
#ifndef CUPCAKE_CSTRING_H
#define CUPCAKE_CSTRING_H

#include "cupcake/text/StringRef.h"
#include "cupcake/util/Error.h"

/*
 * Class to help get a null terminated C string for passing to OS functions.
 */
class CStringBuf {
public:
    CStringBuf(const StringRef path);
    ~CStringBuf();
    
    char* get() const;
    
private:
    char statBuf[128];
    char* strPtr;
};

#ifdef WIN32

/*
 * Windows specific class to help get a null terminated wide C string.
 */
class WCStringBuf {
public:
    WCStringBuf(const StringRef path);
    ~WCStringBuf();

    Error error() const;
    wchar_t* get() const;
    
private:
    wchar_t statBuf[128];
    wchar_t* strPtr;
    Error errVal;
};

/*
 * Windows specific class to help get a null terminated wide C string path
 * for passing to Windows OS functions. Prepends a prefix for longer files
 * if needed.
 *
 * Theoretically, we need a Unix version of this that uses iconv to shift
 * to locale encoding.
 */
class WinPathBuf {
public:
    WinPathBuf(const StringRef path);
    ~WinPathBuf();

    Error error() const;
    wchar_t* get() const;
    
private:
    wchar_t statBuf[128];
    wchar_t* pathPtr;
    Error errVal;
};
#endif

#endif // CUPCAKE_CSTRING_H
