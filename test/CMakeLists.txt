add_subdirectory(unit)
add_subdirectory(unitinc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

add_executable(cupcake_unit
  ${CUPCAKE_SRC}
  ${CUPCAKE_UNIT_SRC}
)
target_include_directories(cupcake_unit PUBLIC
  ${CMAKE_SOURCE_DIR}/cupcake/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/unitinc
)
target_link_libraries(cupcake_unit cupcake ${OS_LIBS})
