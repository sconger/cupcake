
#ifndef CUPCAKE_HANDLER_MAP_H
#define CUPCAKE_HANDLER_MAP_H

#include "cupcake/http/Http.h"
#include "cupcake/text/StringRef.h"

#include "cupcake/internal/util/PathTrie.h"

#include <tuple>

namespace Cupcake {

/*
 * Mapping of URL paths to handlers.
 */
class HandlerMap {
public:
    HandlerMap() = default;

    bool addHandler(const StringRef path, HttpHandler handler);

    std::tuple<HttpHandler, bool> getHandler(const StringRef path) const;

private:
    PathTrie<HttpHandler> handlers;
};

}

#endif // CUPCAKE_HANDLER_MAP_H
