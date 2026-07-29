cmake_minimum_required(VERSION 3.30)

project(LFIF VERSION 2.0.0 LANGUAGES CXX)

option(LFIF_BUILD_EXTRAS "Build codec comparison and benchmark tools" OFF)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED COMPONENTS CXX)

add_subdirectory(liblfif)
add_subdirectory(libppm)
add_subdirectory(tools)

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(LFIF_BUILD_EXTRAS)
  add_subdirectory(extra)
endif()
