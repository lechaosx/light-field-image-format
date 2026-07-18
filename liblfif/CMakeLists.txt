add_library(lfif STATIC src/huffman.cc src/bwt.cc src/runlength.cc)
target_include_directories(lfif PUBLIC include)
target_compile_features(lfif PUBLIC cxx_std_23)
target_link_libraries(lfif PRIVATE OpenMP::OpenMP_CXX)
