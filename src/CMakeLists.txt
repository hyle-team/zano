
if(POLICY CMP0043)
  cmake_policy(SET CMP0043 OLD)
endif()


###########
# using shared PCH -- this is unusual case for MSVC... so mystery, such hack, many wow. See also: https://stackoverflow.com/questions/645747/sharing-precompiled-headers-between-projects-in-visual-studio/4170902#4170902
# define USE_PCH to YES for using precomiled headers

MACRO(INIT_SHARED_PCH pch_cpp_file)
  IF(MSVC AND USE_PCH)
    set_property(SOURCE ${pch_cpp_file} APPEND_STRING PROPERTY COMPILE_FLAGS " /Fo$(OutDir) /Z7 /Fd$(OutDir)vc$(PlatformToolsetVersion).pdb /Ycstdafx.h /Fp$(TargetDir)pch.pch")
  ENDIF(MSVC AND USE_PCH)
ENDMACRO(INIT_SHARED_PCH)

MACRO(ENABLE_SHARED_PCH sources_var)
  IF(MSVC AND USE_PCH)
    MESSAGE( STATUS " ...... enabling precompiled headers for: " ${sources_var} )
    SET(precompiled_binary "$(TargetDir)pch.pch")
    SET(precompiled_header "${CMAKE_CURRENT_SOURCE_DIR}/pch/stdafx.h")
    SET(sources ${${sources_var}})
    foreach(src ${sources})
      if(NOT ${src} MATCHES "\\.rc$") # skip *.rc files
        SET_SOURCE_FILES_PROPERTIES(${src}
                                PROPERTIES COMPILE_FLAGS "/Z7 /Fd$(OutDir)vc$(PlatformToolsetVersion).pdb /Yu\"${precompiled_header}\" /FI\"${precompiled_header}\" /Fp\"${precompiled_binary}\""
                                           OBJECT_DEPENDS "${precompiled_binary}")
      endif()
    endforeach()
  ENDIF(MSVC AND USE_PCH)
ENDMACRO(ENABLE_SHARED_PCH)

MACRO(ENABLE_SHARED_PCH_EXECUTABLE target)
  IF(MSVC AND USE_PCH)
    SET_TARGET_PROPERTIES(${target} PROPERTIES LINK_FLAGS "$(OutDir)stdafx.obj")
  ENDIF(MSVC AND USE_PCH)
ENDMACRO(ENABLE_SHARED_PCH_EXECUTABLE)
###########




file(GLOB_RECURSE PCH pch/*)
file(GLOB_RECURSE COMMON common/*)
file(GLOB CRYPTO crypto/*.*)
file(GLOB_RECURSE CURRENCY_CORE currency_core/*)
file(GLOB_RECURSE CURRENCY_PROTOCOL currency_protocol/*)
file(GLOB_RECURSE DAEMON daemon/*)

file(GLOB_RECURSE P2P p2p/*)
file(GLOB_RECURSE RPC rpc/*)
file(GLOB_RECURSE STRATUM stratum/*)
file(GLOB_RECURSE SIMPLEWALLET simplewallet/*)
file(GLOB_RECURSE CONN_TOOL connectivity_tool/*)
file(GLOB_RECURSE WALLET wallet/*)
file(GLOB_RECURSE MINER miner/*)

if(BUILD_GUI)
  if(MSVC)
    file(GLOB_RECURSE QTDAEMON gui/qt-daemon/*.cpp gui/qt-daemon/*.h gui/qt-daemon/app.rc)
  elseif(APPLE)
    file(GLOB_RECURSE QTDAEMON gui/qt-daemon/*.cpp gui/qt-daemon/*.h gui/qt-daemon/*.mm)
  else()
    file(GLOB_RECURSE QTDAEMON gui/qt-daemon/*.cpp gui/qt-daemon/*.h)    
  endif()
endif()


source_group(pch FILES ${PCH})
source_group(common FILES ${COMMON})
source_group(crypto FILES ${CRYPTO})
source_group(currency_core FILES ${CURRENCY_CORE})
source_group(currency_protocol FILES ${CURRENCY_PROTOCOL})
source_group(daemon FILES ${DAEMON})
source_group(p2p FILES ${P2P})
source_group(rpc FILES ${RPC})
source_group(stratum FILES ${STRATUM})
source_group(simplewallet FILES ${SIMPLEWALLET})
source_group(connectivity-tool FILES ${CONN_TOOL})
source_group(wallet FILES ${WALLET})

if(BUILD_GUI)
  source_group(qtdaemon FILES ${QTDAEMON})
endif()


if (USE_PCH)
  add_library(pch ${PCH})
  set(PCH_LIB_NAME pch)
endif()
INIT_SHARED_PCH("pch/stdafx.cpp")

add_library(common ${COMMON})
add_dependencies(common version ${PCH_LIB_NAME})
ENABLE_SHARED_PCH(COMMON)

if(NOT MSVC AND NOT APPLE AND NOT CLANG)  # TODO(unassigned): do we really need the clang equivalent?
   target_compile_options(common PRIVATE -fno-var-tracking-assignments)
endif()

add_library(crypto ${CRYPTO})

add_library(currency_core ${CURRENCY_CORE})
add_dependencies(currency_core version rpc ${PCH_LIB_NAME})
ENABLE_SHARED_PCH(CURRENCY_CORE)

add_library(rpc ${RPC})
add_dependencies(rpc version ${PCH_LIB_NAME})
ENABLE_SHARED_PCH(RPC)

add_library(stratum ${STRATUM})
add_dependencies(stratum version ${PCH_LIB_NAME})
ENABLE_SHARED_PCH(STRATUM)

add_library(wallet ${WALLET})
add_dependencies(wallet version ${PCH_LIB_NAME})
ENABLE_SHARED_PCH(WALLET)

target_link_libraries(currency_core lmdb mdbx)

add_executable(daemon ${DAEMON} ${P2P} ${CURRENCY_PROTOCOL})
add_dependencies(daemon version)
target_link_libraries(daemon rpc stratum currency_core crypto common libminiupnpc-static zlibstatic ethash ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
ENABLE_SHARED_PCH(DAEMON)
ENABLE_SHARED_PCH_EXECUTABLE(daemon)
                     
add_executable(connectivity_tool ${CONN_TOOL})
add_dependencies(connectivity_tool version)
target_link_libraries(connectivity_tool currency_core crypto common zlibstatic ethash ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
ENABLE_SHARED_PCH(CONN_TOOL)
ENABLE_SHARED_PCH_EXECUTABLE(connectivity_tool)

add_executable(simplewallet ${SIMPLEWALLET})
add_dependencies(simplewallet version)
target_link_libraries(simplewallet wallet rpc currency_core crypto common zlibstatic ethash ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
ENABLE_SHARED_PCH(SIMPLEWALLET)
ENABLE_SHARED_PCH_EXECUTABLE(simplewallet)

set_property(TARGET common crypto currency_core rpc stratum wallet PROPERTY FOLDER "libs")
set_property(TARGET daemon simplewallet connectivity_tool PROPERTY FOLDER "prog")
set_property(TARGET daemon PROPERTY OUTPUT_NAME "zanod")

if(BUILD_GUI)

  if(APPLE)
    FIND_LIBRARY(COCOA_LIBRARY Cocoa)
  endif()

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  SET(MACOSX_BUNDLE_ICON_FILE app.icns)
  add_executable(Zano WIN32 MACOSX_BUNDLE ${QTDAEMON} )
  ENABLE_SHARED_PCH(QTDAEMON)
  ENABLE_SHARED_PCH_EXECUTABLE(Zano)
	  
  QT5_USE_MODULES(Zano WebEngineWidgets WebChannel)
  find_package(Qt5PrintSupport REQUIRED)

  target_link_libraries(Zano wallet rpc currency_core crypto common zlibstatic ethash Qt5::WebEngineWidgets Qt5::PrintSupport ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

  if(APPLE)
    target_link_libraries(Zano ${COCOA_LIBRARY})
  endif()
  if(MSVC)
    target_link_libraries(Zano shlwapi.lib)
  endif()

  set_property(TARGET Zano PROPERTY FOLDER "prog")
  set_target_properties(Zano PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/gui/qt-daemon/Info.plist.in)

  set(HTML_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gui/qt-daemon/html)
  set_target_properties(Zano PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "--html-path=${HTML_DIR}")

  set(CMAKE_AUTOMOC OFF)

  # GUI convenience "bundle"
  # set(GUI_DIR ${CMAKE_CURRENT_BINARY_DIR}/gui)
  # set_target_properties(Zano PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${GUI_DIR})
  # add_custom_command(TARGET Zano POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${HTML_DIR} ${GUI_DIR}/html)
endif()

if(APPLE)
  set(SIMPLE_BUNDLE 1)
endif()

if(SIMPLE_BUNDLE)
  set(INSTALL_DIR "${CMAKE_BINARY_DIR}/hp-${VERSION}")
  install(TARGETS daemon simplewallet connectivity_tool
      RUNTIME DESTINATION "${INSTALL_DIR}" COMPONENT Runtime
  )

  install(FILES ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} DESTINATION "${INSTALL_DIR}/lib")

  if(APPLE)
    set(FIXUP_COMMAND ${CMAKE_SOURCE_DIR}/utils/macosx_fixup.sh " " ${INSTALL_DIR})
    install(CODE "execute_process(COMMAND ${FIXUP_COMMAND})")
  endif()
endif()
