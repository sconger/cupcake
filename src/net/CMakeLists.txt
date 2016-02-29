
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set(LOCAL
  ${CMAKE_CURRENT_SOURCE_DIR}/SocketImpl_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Addrinfo_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/INet_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SockAddr_win.cpp
)
else()

endif()

set(LOCAL
  ${LOCAL}
  ${CMAKE_CURRENT_SOURCE_DIR}/Socket.cpp
)

set(CUPCAKE_SRC
  ${CUPCAKE_SRC}
  ${LOCAL}
  PARENT_SCOPE
)

source_group(src\\net FILES ${LOCAL})
