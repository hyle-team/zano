set(UPNPC_BUILD_STATIC ON CACHE BOOL "Build static library")
set(UPNPC_BUILD_SHARED OFF CACHE BOOL "Build shared library")
set(UPNPC_BUILD_TESTS OFF CACHE BOOL "Build test executables")
add_subdirectory(miniupnpc)
add_subdirectory(zlib)
add_subdirectory(db)



set_property(TARGET upnpc-static PROPERTY FOLDER "contrib")
set_property(TARGET zlibstatic PROPERTY FOLDER "contrib")
set_property(TARGET lmdb PROPERTY FOLDER "contrib")



if(MSVC)
  set_property(TARGET upnpc-static APPEND_STRING PROPERTY COMPILE_FLAGS " -wd4244 -wd4267")
else()
  set_property(TARGET upnpc-static APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-undef -Wno-unused-result -Wno-unused-value")
endif()
