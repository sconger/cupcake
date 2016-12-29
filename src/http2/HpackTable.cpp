
#include "cupcake/internal/http2/HpackTable.h"

#include <cassert>

using namespace Cupcake;

// The spec defines a fixed overhead to assume for each entry
#define ENTRY_OVERHEAD 32

// Maximum index of something in the static table (note that there is no 0 indexed entry)
#define STATIC_TABLE_MAX 61
#define DYNAMIC_TABLE_START 62

// Tables from appendix A of the HPACK spec
static
constexpr StringRef staticNames[STATIC_TABLE_MAX + 1] = {
    nullptr, // There is no 0 indexed entry
    ":authority",
    ":method",
    ":method",
    ":path",
    ":path",
    ":scheme",
    ":scheme",
    ":status",
    ":status",
    ":status",
    ":status",
    ":status",
    ":status",
    ":status",
    "accept-charset",
    "accept-encoding",
    "accept-language",
    "accept-ranges",
    "accept",
    "access-control-allow-origin",
    "age",
    "allow",
    "authorization",
    "cache-control",
    "content-disposition",
    "content-encoding",
    "content-language",
    "content-length",
    "content-location",
    "content-range",
    "content-type",
    "cookie",
    "date",
    "etag",
    "expect",
    "expires",
    "from",
    "host",
    "if-match",
    "if-modified-since",
    "if-none-match",
    "if-range",
    "if-unmodified-since",
    "last-modified",
    "link",
    "location",
    "max-forwards",
    "proxy-authenticate",
    "proxy-authorization",
    "range",
    "referer",
    "refresh",
    "retry-after",
    "server",
    "set-cookie",
    "strict-transport-security",
    "transfer-encoding",
    "user-agent",
    "vary",
    "via",
    "www-authenticate",
};

static
constexpr StringRef staticValues[STATIC_TABLE_MAX + 1] = {
    nullptr, // There is no 0 indexed entry
    "",
    "GET",
    "POST",
    "/",
    "/index.html",
    "http",
    "https",
    "200",
    "204",
    "206",
    "304",
    "400",
    "404",
    "500",
    "",
    "gzip, deflate",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

HpackTable::HpackTable() :
    maxTableBytes(0),
    currentTableBytes(0)
{}

void HpackTable::init(size_t initMaxTableBytes) {
    maxTableBytes = initMaxTableBytes;
}

void HpackTable::resize(size_t newMaxTableBytes) {
    while (currentTableBytes > newMaxTableBytes) {
        dropEntry();
    }
    maxTableBytes = newMaxTableBytes;
}

bool HpackTable::add(const StringRef headerName, const StringRef headerValue) {
    size_t entrySize = headerName.length() + headerValue.length() + ENTRY_OVERHEAD;

    if (entrySize > maxTableBytes) {
        return false;
    }

    while (currentTableBytes + entrySize > maxTableBytes) {
        dropEntry();
    }
    currentTableBytes += entrySize;
    dynamicTable.emplace_front(Entry(headerName, headerValue));
    return true;
}

void HpackTable::dropEntry() {
    const Entry& lastEntry = dynamicTable.back();
    size_t entrySize = lastEntry.name.length() + lastEntry.value.length() + ENTRY_OVERHEAD;
    currentTableBytes -= entrySize;
    dynamicTable.pop_back();
}

bool HpackTable::hasEntryAtIndex(size_t index) const {
    assert(index != 0);

    if (index <= STATIC_TABLE_MAX) {
        return true;
    }
    return dynamicTable.size() >= (index - STATIC_TABLE_MAX);
}

const StringRef HpackTable::nameAtIndex(size_t index) const {
    assert(index != 0);

    if (index <= STATIC_TABLE_MAX) {
        return staticNames[index];
    }
    return dynamicTable[index - DYNAMIC_TABLE_START].name;
}

const StringRef HpackTable::valueAtIndex(size_t index) const {
    assert(index != 0);

    if (index <= STATIC_TABLE_MAX) {
        return staticValues[index];
    }
    return dynamicTable[index - DYNAMIC_TABLE_START].value;
}
