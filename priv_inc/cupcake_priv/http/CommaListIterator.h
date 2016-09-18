
#ifndef CUPCAKE_HTTP_COMMA_LIST_ITERATOR
#define CUPCAKE_HTTP_COMMA_LIST_ITERATOR

#include "cupcake/text/StringRef.h"

namespace Cupcake {

/*
 * Helper class for parsing HTTP comma delimited lists.
 */
class CommaListIterator {
public:
    CommaListIterator(const StringRef text);

    StringRef next();
    StringRef getLast();

private:
    CommaListIterator(const CommaListIterator&) = delete;
    CommaListIterator& operator=(const CommaListIterator&) = delete;

    StringRef text;
};

}

#endif // CUPCAKE_HTTP_COMMA_LIST_ITERATOR
