add_library(
  lfif_tool_io
  STATIC
  src/file_mask.cc
  src/plenoppm.cc
)
target_include_directories(lfif_tool_io PUBLIC include)
target_link_libraries(lfif_tool_io PUBLIC ppm PRIVATE lfif_project_options)

add_executable(
  lfif2d_compress
  src/compress.cc
  src/lfif2d_compress.cc
)
target_include_directories(lfif2d_compress PRIVATE include)
target_link_libraries(lfif2d_compress PRIVATE lfif ppm lfif_project_options)

add_executable(
  lfif2d_decompress
  src/decompress.cc
  src/lfif2d_decompress.cc
)
target_include_directories(lfif2d_decompress PRIVATE include)
target_link_libraries(lfif2d_decompress PRIVATE lfif ppm lfif_project_options)

add_executable(
  lfif4d_compress
  src/compress.cc
  src/lfif4d_compress.cc
)
target_include_directories(lfif4d_compress PRIVATE include)
target_link_libraries(
  lfif4d_compress
  PRIVATE lfif lfif_tool_io ppm lfif_project_options
)

add_executable(
  lfif4d_decompress
  src/decompress.cc
  src/lfif4d_decompress.cc
)
target_include_directories(lfif4d_decompress PRIVATE include)
target_link_libraries(
  lfif4d_decompress
  PRIVATE lfif lfif_tool_io ppm lfif_project_options
)

install(
  TARGETS
    lfif2d_compress
    lfif2d_decompress
    lfif4d_compress
    lfif4d_decompress
  RUNTIME DESTINATION bin
)
