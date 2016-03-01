file(GLOB
  LOCAL
  "*.cpp"
)

set(CUPCAKE_SRC
  ${CUPCAKE_SRC}
  ${LOCAL}
  PARENT_SCOPE
)
