add_subdirectory(async)
add_subdirectory(http)
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
