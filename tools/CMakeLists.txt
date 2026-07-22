add_executable(lfif2d_compress
  src/compress.cc
  src/lfif2d_compress.cc
)

add_executable(lfif2d_decompress
  src/decompress.cc
  src/lfif2d_decompress.cc
)

add_executable(lfif4d_compress
  src/compress.cc
  src/file_mask.cc
  src/lfif4d_compress.cc
  src/plenoppm.cc
)

add_executable(lfif4d_decompress
  src/decompress.cc
  src/file_mask.cc
  src/lfif4d_decompress.cc
  src/plenoppm.cc
)

foreach(target lfif2d_compress lfif2d_decompress lfif4d_compress lfif4d_decompress)
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(${target} PRIVATE lfif ppm)
endforeach()
