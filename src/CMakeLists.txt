add_subdirectory(async)
add_subdirectory(http)
add_subdirectory(net)
add_subdirectory(text)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(LOCAL
    ${CMAKE_CURRENT_SOURCE_DIR}/Cupcake_win.cpp
  )
else()
  set(LOCAL
    ${CMAKE_CURRENT_SOURCE_DIR}/Cupcake_unix.cpp
  )
endif()

set(LOCAL
  ${LOCAL}
  ${CMAKE_CURRENT_SOURCE_DIR}/CString.cpp
)

set(CUPCAKE_SRC
  ${CUPCAKE_SRC}
  ${LOCAL}
  PARENT_SCOPE
)
