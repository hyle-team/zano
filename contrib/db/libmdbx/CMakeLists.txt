
set (mdbx_sources src/mdbx.c src/osal.c)

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
