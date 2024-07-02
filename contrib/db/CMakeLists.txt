

if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "Android")
  message("excluded db support for IOS build")
  return()
endif()


message("DB ENGINE: lmdb")  
add_subdirectory(liblmdb)
if(MSVC)
   target_compile_options(lmdb PRIVATE /wd4996 /wd4503 /wd4345 /wd4267 /wd4244 /wd4146 /wd4333 /wd4172)
else()
   # Warnings as used by LMDB itself (LMDB_0.9.23)
   target_compile_options(lmdb PRIVATE -Wall -Wno-unused-parameter -Wbad-function-cast -Wuninitialized)
endif()

if(NOT DISABLE_MDBX)
   message("DB ENGINE: mdbx")  
   add_subdirectory(libmdbx)
   # remove mdbx tools from the default MSVC build
   set_target_properties(mdbx_chk mdbx_copy mdbx_dump mdbx_load mdbx_stat PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
endif()
