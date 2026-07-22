cmake_minimum_required(VERSION 3.24)

project(LFIF VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wfatal-errors
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Release>:-march=native>
  )
endif()

add_subdirectory(liblfif)
add_subdirectory(libppm)
add_subdirectory(tools)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
