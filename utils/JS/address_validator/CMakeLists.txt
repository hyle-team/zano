cmake_minimum_required(VERSION 3.16)

project(zano_addr_validator_wasm LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(ZANO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

set(IS_EMSCRIPTEN OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(IS_EMSCRIPTEN ON)
endif()

set(Boost_INCLUDE_DIR "" CACHE PATH "Path to Boost headers (e.g. /home/user/boost_1_84_0)")
if(Boost_INCLUDE_DIR STREQUAL "")
  message(FATAL_ERROR "Boost_INCLUDE_DIR is required (path to Boost headers).")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_MINIZIP OFF CACHE BOOL "" FORCE)

if(EXISTS "${ZANO_ROOT}/contrib/zlib/CMakeLists.txt")
  add_subdirectory("${ZANO_ROOT}/contrib/zlib" "${CMAKE_BINARY_DIR}/contrib/zlib")
else()
  message(FATAL_ERROR "contrib/zlib not found at: ${ZANO_ROOT}/contrib/zlib")
endif()

set(ZANO_ZLIB_TARGET "")
if(TARGET zlibstatic)
  set(ZANO_ZLIB_TARGET zlibstatic)
elseif(TARGET zlib)
  set(ZANO_ZLIB_TARGET zlib)
else()
  message(FATAL_ERROR "zlib target not found after add_subdirectory(contrib/zlib)")
endif()

set(ZANO_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${ZANO_GENERATED_DIR}")

set(ZANO_VERSION_HEADER_DIR "")
if(EXISTS "${ZANO_ROOT}/src/version.h")
  set(ZANO_VERSION_HEADER_DIR "${ZANO_ROOT}/src")
else()
  file(WRITE "${ZANO_GENERATED_DIR}/version.h"
"#pragma once\n"
"#define PROJECT_VERSION_MAJOR 0\n"
"#define PROJECT_VERSION_MINOR 0\n"
"#define PROJECT_VERSION_PATCH 0\n"
"#define PROJECT_VERSION_BUILD 0\n"
"#define PROJECT_VERSION_STRING \"0.0.0\"\n"
  )
  set(ZANO_VERSION_HEADER_DIR "${ZANO_GENERATED_DIR}")
endif()

set(ZANO_EOS_INCLUDE_DIR "${ZANO_ROOT}/contrib/eos_portable_archive")
if(NOT EXISTS "${ZANO_EOS_INCLUDE_DIR}/eos/portable_archive.hpp")
  message(FATAL_ERROR "Expected ${ZANO_EOS_INCLUDE_DIR}/eos/portable_archive.hpp not found")
endif()

add_library(zano_includes INTERFACE)

target_include_directories(zano_includes INTERFACE
  "${ZANO_ROOT}/src"
  "${ZANO_ROOT}/contrib"
  "${ZANO_ROOT}/contrib/epee/include"
  "${ZANO_ROOT}/contrib/jwt-cpp/include"
  "${ZANO_ROOT}/contrib/zlib"
  "${ZANO_VERSION_HEADER_DIR}"
  "${Boost_INCLUDE_DIR}"
  "${ZANO_EOS_INCLUDE_DIR}"
)

if(IS_EMSCRIPTEN)
  target_compile_options(zano_includes INTERFACE -pthread)
  target_link_options(zano_includes INTERFACE -pthread)
endif()

file(GLOB_RECURSE COMMON_SRC CONFIGURE_DEPENDS
  "${ZANO_ROOT}/src/common/*.c"
  "${ZANO_ROOT}/src/common/*.cpp"
)

list(FILTER COMMON_SRC EXCLUDE REGEX ".*/db_backend_.*\\.cpp$")
list(FILTER COMMON_SRC EXCLUDE REGEX ".*/ntp\\.cpp$")
list(FILTER COMMON_SRC EXCLUDE REGEX ".*/command_line\\.cpp$")
list(FILTER COMMON_SRC EXCLUDE REGEX ".*/stack_trace_.*\\.cpp$")
list(FILTER COMMON_SRC EXCLUDE REGEX ".*/miniupnp_helper\\.cpp$")

file(GLOB_RECURSE CRYPTO_SRC CONFIGURE_DEPENDS
  "${ZANO_ROOT}/src/crypto/*.c"
  "${ZANO_ROOT}/src/crypto/*.cpp"
)

list(FILTER CRYPTO_SRC EXCLUDE REGEX ".*/eth_signature\\.cpp$")
list(FILTER CRYPTO_SRC EXCLUDE REGEX ".*/ethereum/.*")

file(GLOB_RECURSE CURRENCY_CORE_SRC CONFIGURE_DEPENDS
  "${ZANO_ROOT}/src/currency_core/*.c"
  "${ZANO_ROOT}/src/currency_core/*.cpp"
)

list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/pos_mining\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/miner\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/basic_pow_helpers\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/blockchain_storage\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/currency_core\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/tx_pool\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/checkpoints\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/tx_semantic_validation\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/bc_.*service.*\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/core_default_rpc_proxy.*\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/rpc_.*\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/wallet_.*\\.cpp$")
list(FILTER CURRENCY_CORE_SRC EXCLUDE REGEX ".*/net_.*\\.cpp$")

# emscripten/libc++ workaround (no repo source edits)
if(IS_EMSCRIPTEN)
  set(WASM_PREINCLUDE "${CMAKE_BINARY_DIR}/wasm_preinclude.hpp")
  file(WRITE "${WASM_PREINCLUDE}"
"// Forced preinclude for Emscripten/libc++ build.\n"
"// libc++ forbids user specializations of std::is_pod.\n"
"#pragma once\n"
"#ifdef MARK_AS_POD_C11\n"
"#undef MARK_AS_POD_C11\n"
"#endif\n"
"#define MARK_AS_POD_C11(type) /* no-op for libc++ */\n"
  )
endif()

add_library(common STATIC ${COMMON_SRC})
target_link_libraries(common PUBLIC zano_includes ${ZANO_ZLIB_TARGET} Threads::Threads)

add_library(crypto STATIC ${CRYPTO_SRC})
target_link_libraries(crypto PUBLIC zano_includes ${ZANO_ZLIB_TARGET} Threads::Threads)

add_library(currency_core_min STATIC ${CURRENCY_CORE_SRC})
target_link_libraries(currency_core_min PUBLIC zano_includes crypto common ${ZANO_ZLIB_TARGET} Threads::Threads)

# apply forced preinclude and warning suppressions for emscripten
if(IS_EMSCRIPTEN)
  foreach(t common crypto currency_core_min)
    target_compile_options(${t} PRIVATE
      "-include" "${WASM_PREINCLUDE}"
      -Wno-invalid-specialization
      -Wno-error=invalid-specialization
    )
  endforeach()
endif()

add_executable(address_validator_wasm
  "${CMAKE_CURRENT_LIST_DIR}/wasm_api.cpp"
)
target_link_libraries(address_validator_wasm PRIVATE currency_core_min crypto common Threads::Threads)

if(IS_EMSCRIPTEN)
  target_compile_options(address_validator_wasm PRIVATE
    -pthread
    "-include" "${WASM_PREINCLUDE}"
    -Wno-invalid-specialization
    -Wno-error=invalid-specialization
  )

  target_link_options(address_validator_wasm PRIVATE
    -pthread
    "-sUSE_PTHREADS=1"
    "-sPTHREAD_POOL_SIZE=4"
    "-sWASM=1"
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=ZanoAddrValidator"
    "-sEXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\"]"
    "-sEXPORTED_FUNCTIONS=[\"_zano_validate_address\"]"
    "-Wl,--export=zano_validate_address"
    "-sALLOW_MEMORY_GROWTH=1"
  )
endif()
