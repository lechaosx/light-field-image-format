add_library(ppm STATIC src/ppm.cc)

target_include_directories(
  ppm
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
