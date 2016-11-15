
#ifndef CUPCAKE_HPACK_TABLE_H
#define CUPCAKE_HPACK_TABLE_H

#include "cupcake/text/String.h"

#include <deque>

namespace Cupcake {

class HpackTable {
public:
    HpackTable();
    void init(size_t maxTableBytes);
    void resize(size_t newMaxTableBytes);

    bool add(const StringRef headerName, const StringRef headerValue);

    bool hasEntryAtIndex(size_t index) const;
    const StringRef nameAtIndex(size_t index) const;
    const StringRef valueAtIndex(size_t index) const;

private:
    HpackTable(const HpackTable&) = delete;
    HpackTable& operator=(const HpackTable&) = delete;

    void dropEntry();

    class Entry {
    public:
        Entry(const StringRef name, const StringRef value) :
            name(name), value(value) {}

        String name;
        String value;
    };

    size_t maxTableBytes;
    size_t currentTableBytes;
    std::deque<Entry> dynamicTable;
};

}

#endif // CUPCAKE_HPACK_TABLE_H
