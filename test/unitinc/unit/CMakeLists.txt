
add_subdirectory(net)
add_subdirectory(text)

file(GLOB
  LOCAL
  "*.h"
)

set(CUPCAKE_UNIT_SRC
  ${CUPCAKE_UNIT_SRC}
  ${LOCAL}
  PARENT_SCOPE
)

source_group(inc\\unit FILES ${LOCAL})
