cmake_minimum_required(VERSION 3.18)

project(LFIF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -pedantic -Wfatal-errors)
add_compile_options(
  $<$<CONFIG:Debug>:-Og>
  $<$<CONFIG:Release>:-march=native>
)

find_package(OpenMP REQUIRED)

add_subdirectory(liblfif)
add_subdirectory(libppm)
add_subdirectory(tools)

add_custom_target(doc COMMAND doxygen ../Doxyfile
  COMMENT "Generating API documentation with Doxygen")
