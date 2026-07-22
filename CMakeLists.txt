cmake_minimum_required(VERSION 3.25)

project(LFIF VERSION 2.0.0 LANGUAGES CXX)

option(LFIF_BUILD_EXTRAS "Build codec comparison and benchmark tools" OFF)
option(LFIF_ENABLE_OPENMP "Enable OpenMP acceleration" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(lfif_project_options INTERFACE)
target_compile_features(lfif_project_options INTERFACE cxx_std_20)
target_compile_options(
  lfif_project_options
  INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wpedantic>
)

if(LFIF_ENABLE_OPENMP)
  find_package(OpenMP QUIET COMPONENTS CXX)

  if(NOT OpenMP_CXX_FOUND AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_library(OpenMP::OpenMP_CXX INTERFACE IMPORTED)
    target_compile_options(OpenMP::OpenMP_CXX INTERFACE -fopenmp)
    target_link_options(OpenMP::OpenMP_CXX INTERFACE -fopenmp)
    set(OpenMP_CXX_FOUND TRUE)
  endif()

  if(NOT OpenMP_CXX_FOUND)
    message(FATAL_ERROR "LFIF_ENABLE_OPENMP is ON, but OpenMP was not found")
  endif()
endif()

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

install(
  TARGETS lfif ppm
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY liblfif/include/ DESTINATION include)
install(DIRECTORY libppm/include/ DESTINATION include)
