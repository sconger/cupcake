
#include "cupcake/internal/http/HandlerMap.h"

using namespace Cupcake;

bool HandlerMap::addHandler(const StringRef path, HttpHandler handler) {
    return handlers.addNode(path, handler);
}

std::tuple<HttpHandler, bool> HandlerMap::getHandler(const StringRef path) const {
    return handlers.find(path);
}