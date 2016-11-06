
file(GLOB_RECURSE CUPCAKE_UNIT_SRC
  unit/*.cpp
  unitinc/*.h)
#file(GLOB_RECURSE CUPCAKE_UNIT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/unit *.cpp *.h)
#file(GLOB_RECURSE CUPCAKE_UNIT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/unitinc *.cpp *.h)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

add_executable(cupcake_unit
  ${CUPCAKE_UNIT_SRC}
)
target_include_directories(cupcake_unit PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../cupcake/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/unitinc
)
target_link_libraries(cupcake_unit cupcake ${OS_LIBS})

_gen_source_group("${CUPCAKE_UNIT_SRC}")
