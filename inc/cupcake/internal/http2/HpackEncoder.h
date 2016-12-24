
#ifndef CUPCAKE_HPACK_ENCODER_H
#define CUPCAKE_HPACK_ENCODER_H

#include "cupcake/internal/http/RequestData.h"
#include "cupcake/internal/http2/HpackTable.h"

/*
 * Encodes header data into HPACK format.
 */
class HpackEncoder {
public:

private:
    HpackEncoder(const HpackEncoder&) = delete;
    HpackEncoder& operator=(const HpackEncoder&) = delete;
};

#endif // CUPCAKE_HPACK_ENCODER_H
