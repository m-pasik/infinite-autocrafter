cmake_minimum_required(VERSION 4.0.3)
project("Infinite AutoCrafter" VERSION 0.0.1 DESCRIPTION "Crafts random elements in Infinite Craft" LANGUAGES C)

option(ADDRESS_SANITIZER "Enable address sanitizer." OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

string(APPEND CMAKE_C_FLAGS_DEBUG " -Wall -O0 -g")
string(APPEND CMAKE_C_FLAGS_RELEASE " -Wall -O3")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)
else()
    message(FATAL_ERROR "ERROR: Unsupported build type. Set CMAKE_BUILD_TYPE to Release/Debug.")
endif()

if (ADDRESS_SANITIZER)
    string(APPEND CMAKE_C_FLAGS " -fno-omit-frame-pointer -fsanitize=address")
endif()

string(APPEND CMAKE_C_FLAGS " -mrdseed")

find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(JSONC REQUIRED)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(infinite-autocrafter ${SOURCES})
target_link_libraries(infinite-autocrafter PRIVATE CURL::libcurl ZLIB::ZLIB JSONC::JSONC)
