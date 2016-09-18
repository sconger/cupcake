
#include "cupcake_priv/http/CommaListIterator.h"

using namespace Cupcake;

/* Helper to trim HTTP header whitespace. RFC 7230 defines that as spaces or horizontal tabs. */
static
StringRef trimWhitespace(const StringRef strRef) {
    // Prevent underflow
    if (strRef.length() == 0) {
        return StringRef("", 0);
    }

    size_t len = strRef.length();
    size_t startIndex = 0;

    while (startIndex < len &&
           (strRef.charAt(startIndex) == ' ' ||
            strRef.charAt(startIndex) == '\t')) {
        startIndex++;
    }

    size_t endIndex = strRef.length() - 1;

    while (endIndex > startIndex &&
           (strRef.charAt(endIndex) == ' ' ||
            strRef.charAt(endIndex) == '\t')) {
        endIndex--;
    }

    return strRef.substring(startIndex, endIndex+1);
}

CommaListIterator::CommaListIterator(const StringRef text) :
    text(text)
{}

StringRef CommaListIterator::next() {
    StringRef ret;
    ptrdiff_t commaIndex = text.indexOf(',');
    if (commaIndex == -1) {
        ret = trimWhitespace(text);
        text = StringRef("", 0);
    } else {
        ret = trimWhitespace(text.substring(0, commaIndex));
        text = text.substring(commaIndex + 1);
    }
    return ret;
}

StringRef CommaListIterator::getLast() {
    StringRef res;
    do {
        res = next();
    } while (text.length() != 0);
    return res;
}
