add_library(
  lfif
  STATIC
  src/bwt.cc
  src/huffman.cc
  src/runlength.cc
)

target_include_directories(
  lfif
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(lfif PRIVATE lfif_project_options)
target_compile_features(lfif PUBLIC cxx_std_20)

if(LFIF_ENABLE_OPENMP)
  target_link_libraries(lfif PUBLIC OpenMP::OpenMP_CXX)
endif()
