
add_subdirectory(net)
add_subdirectory(text)

file(GLOB
  LOCAL
  "*.cpp"
)

set(CUPCAKE_UNIT_SRC
  ${CUPCAKE_UNIT_SRC}
  ${LOCAL}
  PARENT_SCOPE
)
