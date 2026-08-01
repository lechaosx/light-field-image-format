add_library(
  lfif
  STATIC
  src/bwt.cc
  src/huffman.cc
)

target_include_directories(
  lfif
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(lfif PUBLIC OpenMP::OpenMP_CXX)
