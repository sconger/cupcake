
#ifndef CUPCAKE_HTTP_ERROR_H
#define CUPCAKE_HTTP_ERROR_H

#include <cstdint>

namespace Cupcake {

/*
 * Error enum for dealing with HTTP streams.
 */
enum class HttpError : uint32_t {
    Ok = 0,

    InvalidState,
    ConnectionClosed,
    IoError,
};

}

#endif // CUPCAKE_HTTP_ERROR_H
