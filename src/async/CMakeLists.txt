
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set(LOCAL
  ${CMAKE_CURRENT_SOURCE_DIR}/Async_win.cpp
)
else()

endif()

set(CUPCAKE_SRC
  ${CUPCAKE_SRC}
  ${LOCAL}
  PARENT_SCOPE
)
