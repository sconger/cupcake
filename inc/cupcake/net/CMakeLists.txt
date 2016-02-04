if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set(LOCAL
  ${CMAKE_CURRENT_SOURCE_DIR}/SocketImpl_win.h
)
else()

endif()

set(LOCAL
  ${LOCAL}
  ${CMAKE_CURRENT_SOURCE_DIR}/Addrinfo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/INet.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Sockaddr.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Socket.h
)

set(CUPCAKE_SRC
  ${CUPCAKE_SRC}
  ${LOCAL}
  PARENT_SCOPE
)

source_group(inc\\cupcake\\net FILES ${LOCAL})
