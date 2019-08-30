
if(WIN32)
  message(STATUS "MDBX locks: lck-windows.c")
  set (mdbx_sources src/mdbx.c src/osal.c src/lck-windows.c )
elseif(APPLE)
  message(STATUS "MDBX locks: lck-posix.c")
  set (mdbx_sources src/mdbx.c src/osal.c src/lck-posix.c )
else()
  message(STATUS "MDBX locks: lck-linux.c")
  set (mdbx_sources src/mdbx.c src/osal.c src/lck-linux.c )
endif()



include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT MSVC)
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers -Wno-missing-braces -Wno-aggregate-return")
endif()

if(FREEBSD)
  # add_definitions(-DMDB_DSYNC=O_SYNC)
endif()

add_library(mdbx  ${mdbx_sources})

target_link_libraries(mdbx PRIVATE ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
  target_link_libraries(mdbx ntdll)
endif()
