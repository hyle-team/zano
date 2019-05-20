set(lmdb_dir ${CMAKE_CURRENT_SOURCE_DIR}/lmdb/libraries/liblmdb)
set(lmdb_sources ${lmdb_dir}/mdb.c ${lmdb_dir}/midl.c)

add_library(lmdb ${lmdb_sources})

target_include_directories(lmdb PRIVATE ${lmdb_dir})
target_link_libraries(lmdb PRIVATE ${CMAKE_THREAD_LIBS_INIT})

# TODO(unassigned): modern CMake
if(NOT MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers -Wno-missing-braces -Wno-aggregate-return")
endif()

if(FREEBSD)
  target_compile_definitions(lmdb PRIVATE "-DMDB_DSYNC=O_SYNC")
endif()

if(WIN32)
  target_link_libraries(lmdb ntdll)
endif()

if(MSVC)
   target_compile_options(lmdb PRIVATE /wd4996 /wd4503 /wd4345 /wd4267 /wd4244 /wd4146 /wd4333 /wd4172)
else()
  # Warnings as used by LMDB itself (LMDB_0.9.23)
  target_compile_options(lmdb PRIVATE -Wall -Wno-unused-parameter -Wbad-function-cast -Wuninitialized)
endif()
