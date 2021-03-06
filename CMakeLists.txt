cmake_minimum_required(VERSION 2.8.12)
project(cupcake CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# http://stackoverflow.com/questions/31966135/cmake-source-group-not-working-correctly-with-hierarchical-project-setup
function(_gen_source_group FILES)
  foreach(FILE ${FILES}) 
    # Get the directory of the source file
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    # Remove common directory prefix to make the group
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    # Make sure we are using windows slashes
    string(REPLACE "/" "\\" GROUP "${GROUP}")

    if (MSVC)
      # Group into "Source Files" and "Header Files"
      if ("${FILE}" MATCHES ".*\\.cpp")
        set(GROUP "Source Files${GROUP}")
      elseif("${FILE}" MATCHES ".*\\.h")
        set(GROUP "Header Files${GROUP}")
      endif()
    else()
      if ("${FILE}" MATCHES ".*\\.cpp")
        set(GROUP "${GROUP}")
      elseif("${FILE}" MATCHES ".*\\.h")
        set(GROUP "${GROUP}")
      endif()
    endif()
    source_group("${GROUP}" FILES "${FILE}")
  endforeach()
endfunction(_gen_source_group)

function(_add_dir_files LIST DIRNAME)
  foreach(FILE ${FILES}) 
    # Get the directory of the source file
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    # Remove common directory prefix to make the group
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    # Make sure we are using windows slashes
    string(REPLACE "/" "\\" GROUP "${GROUP}")

    if (MSVC)
      # Group into "Source Files" and "Header Files"
      if ("${FILE}" MATCHES ".*\\.cpp")
        set(GROUP "Source Files${GROUP}")
      elseif("${FILE}" MATCHES ".*\\.h")
        set(GROUP "Header Files${GROUP}")
      endif()
    else()
      if ("${FILE}" MATCHES ".*\\.cpp")
        set(GROUP "${GROUP}")
      elseif("${FILE}" MATCHES ".*\\.h")
        set(GROUP "${GROUP}")
      endif()
    endif()
    source_group("${GROUP}" FILES "${FILE}")
  endforeach()
endfunction(_add_dir_files)

if(MSVC)
  set(CMAKE_CXX_FLAGS "/await /EHsc")
else()
  set(CMAKE_CXX_FLAGS "-std=c++14 -fcoroutines-ts -Wall")

  # Override default stdlib
  # Obviously, need to update the paths here to make clang work
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/Users/sconger/local/include/c++/v1")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/Users/sconger/local/lib")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath,/Users/sconger/local/lib")
  #set(CMAKE_CXX_FLAGS_DEBUG "-g")
  #set(CMAKE_CXX_FLAGS_RELEASE "-O2 -flto -DNDEBUG")

  # On Windows, we need to set the Windows version
  if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWINVER=0x0601")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")
  endif()

  # TODO: Document how to do this instead of hard coding it here
  # Generate a spec with "g++ -dumpspecs > msvcr100.spec" and modify.
  #if(MSYS)
  #  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -specs=${CMAKE_CURRENT_SOURCE_DIR}/msvcr100.spec")
  #endif()
endif()

if(WIN32)
  set(OS_LIBS Ws2_32 Mswsock)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(OS_LIBS -lpthread)
endif()

file(GLOB_RECURSE CUPCAKE_SRC
  src/*.cpp
  inc/*.h)

add_subdirectory(test)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

add_library(cupcake ${CUPCAKE_SRC})

target_include_directories(cupcake PUBLIC
  inc
  priv_inc
)

_gen_source_group("${CUPCAKE_SRC}")
