file(GLOB
  LOCAL
  "*.h"
)

set(CUPCAKE_SRC
  ${CUPCAKE_SRC}
  ${LOCAL}
  PARENT_SCOPE
)

source_group(inc\\cupcake\\text FILES ${LOCAL})
