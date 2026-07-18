set(COMPRESS_SOURCES   src/compress.cc   src/lfif4d_compress.cc   src/plenoppm.cc src/file_mask.cc)
set(DECOMPRESS_SOURCES src/decompress.cc src/lfif4d_decompress.cc src/plenoppm.cc src/file_mask.cc)

add_executable(lfif2d_compress src/compress.cc src/lfif2d_compress.cc)
add_executable(lfif2d_decompress src/decompress.cc src/lfif2d_decompress.cc)
add_executable(lfif4d_compress ${COMPRESS_SOURCES})
add_executable(lfif4d_decompress ${DECOMPRESS_SOURCES})

foreach(target lfif2d_compress lfif2d_decompress lfif4d_compress lfif4d_decompress)
  target_include_directories(${target} PRIVATE include)
  target_link_libraries(${target} PRIVATE lfif ppm OpenMP::OpenMP_CXX)
endforeach()
