
#ifndef CUPCAKE_CUPCAKE_H
#define CUPCAKE_CUPCAKE_H

// Prevent Windows.h from defining min/max
#ifdef _WIN32
#define NOMINMAX
#endif

#include <cstdint>
#include <climits>

#define MIN_INT8 -128
#define MAX_INT8 127
#define MIN_INT16 –32768
#define MAX_INT16 –32767
#define MIN_INT32 (-2147483647 - 1)
#define MAX_INT32 2147483647
#define MIN_INT64 (-9223372036854775807LL - 1)
#define MAX_INT64 9223372036854775807LL

#define MAX_UINT8 255
#define MAX_UINT16 65535
#define MAX_UINT32 4294967295
#define MAX_UINT64 18446744073709551615ULL

// STR is a macro to convert a macro value to a string
#define _QUOTE(name) #name
#define STR(macro) _QUOTE(macro)

// Library init and shutdown
namespace Cupcake {
    void init();
    void cleanup();
}

#endif // CUPCAKE_CUPCAKE_H
