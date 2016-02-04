add_subdirectory(net)
add_subdirectory(text)
add_subdirectory(util)

file(GLOB
  LOCAL
  "*.h"
)

set(CUPCAKE_SRC
  ${CUPCAKE_SRC}
  ${LOCAL}
  PARENT_SCOPE
)

source_group(inc\\cupcake FILES ${LOCAL})
