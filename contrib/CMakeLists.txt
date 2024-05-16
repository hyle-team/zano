set(UPNPC_BUILD_STATIC ON CACHE BOOL "Build static library")
set(UPNPC_BUILD_SHARED OFF CACHE BOOL "Build shared library")
set(UPNPC_BUILD_TESTS OFF CACHE BOOL "Build test executables")
add_subdirectory(zlib)
add_subdirectory(db)
add_subdirectory(ethereum)

if( NOT DISABLE_TOR)
  add_subdirectory(tor-connect)
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "Android")
  message("excluded upnp support for IOS build")
  return()
endif()

add_subdirectory(miniupnp/miniupnpc)



set_property(TARGET libminiupnpc-static PROPERTY FOLDER "contrib")
set_property(TARGET zlibstatic PROPERTY FOLDER "contrib")
set_property(TARGET mdbx PROPERTY FOLDER "contrib")
set_property(TARGET lmdb PROPERTY FOLDER "contrib")
if( NOT DISABLE_TOR)
  set_property(TARGET tor-connect PROPERTY FOLDER "contrib")
endif()

set_property(TARGET upnpc-static mdbx_chk mdbx_copy mdbx_dump mdbx_load mdbx_stat PROPERTY FOLDER "unused")

if(MSVC)
  #set_property(TARGET ntdll_extra_target PROPERTY FOLDER "unused")
endif()


if(MSVC)
  set_property(TARGET upnpc-static APPEND_STRING PROPERTY COMPILE_FLAGS " /wd4244 /wd4267")
  set_property(TARGET zlibstatic APPEND_STRING PROPERTY COMPILE_FLAGS " /wd4267 /wd4267")
else()
  set_property(TARGET upnpc-static APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-undef -Wno-unused-result -Wno-unused-value -Wno-implicit-fallthrough -Wno-discarded-qualifiers ")
  set_property(TARGET zlibstatic APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-undef -Wno-unused-result -Wno-unused-value -Wno-implicit-fallthrough -Wno-discarded-qualifiers ")
endif()
