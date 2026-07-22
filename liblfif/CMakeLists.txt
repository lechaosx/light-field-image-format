add_library(lfif STATIC
  src/bwt.cc
  src/huffman.cc
  src/runlength.cc
)
target_include_directories(lfif PUBLIC include)
target_compile_features(lfif PUBLIC cxx_std_23)
