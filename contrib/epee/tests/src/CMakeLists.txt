cmake_minimum_required(VERSION 2.8)
project(epee_tests C CXX)

set(Boost_USE_MULTITHREADED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

include_directories(.)
include_directories(../../include)
include_directories(../../../)

find_package(Boost REQUIRED COMPONENTS system filesystem thread date_time chrono regex)
include_directories(${Boost_INCLUDE_DIRS})
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

if (TARGET OpenSSL::SSL)
  set(OPENSSL_LIBS OpenSSL::SSL OpenSSL::Crypto)
else()
  set(OPENSSL_LIBS ${OPENSSL_LIBRARIES})
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if (MSVC)
  add_definitions("/W3 /D_CRT_SECURE_NO_WARNINGS /wd4996 /wd4345 /nologo /D_WIN32_WINNT=0x0600 /DWIN32_LEAN_AND_MEAN /bigobj")
  include_directories(SYSTEM platform/msvc)
else()
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -std=c11 -Wall -Werror")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes") # needed for AES
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17 -Wall -Werror -Wno-error=comment -Wno-reorder")
endif()

add_subdirectory(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../zlib
  ${CMAKE_CURRENT_BINARY_DIR}/zlib_build
  EXCLUDE_FROM_ALL
)

file(GLOB_RECURSE SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/*.inl
     ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

source_group(general FILES ${SRC})

add_executable(tests ${SRC})
target_link_libraries(tests PRIVATE
  zlibstatic
  ${Boost_LIBRARIES}
  Threads::Threads
  ${OPENSSL_LIBS}
)

add_executable(socks5_probe
  net/test_http_socks5.cpp
  net/test_http_socks5_main.cpp
)

target_include_directories(socks5_probe PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../
  ${Boost_INCLUDE_DIRS}
)

if (MSVC)
  target_compile_features(socks5_probe PRIVATE cxx_std_17)
else()
  target_compile_options(socks5_probe PRIVATE -std=gnu++17 -Wno-error=comment)
endif()

target_link_libraries(socks5_probe PRIVATE
  zlibstatic
  ${Boost_LIBRARIES}
  Threads::Threads
  ${OPENSSL_LIBS}
)
