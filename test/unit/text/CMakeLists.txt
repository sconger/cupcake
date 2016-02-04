file(GLOB
  LOCAL
  "*.cpp"
)

set(CUPCAKE_UNIT_SRC
  ${CUPCAKE_UNIT_SRC}
  ${LOCAL}
  PARENT_SCOPE
)

source_group(src\\unit\\text FILES ${LOCAL})
