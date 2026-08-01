if(LFIF_BUILD_EXTRAS)
  add_library(
    lfif_tool_io
    STATIC
    src/file_mask.cc
    src/plenoppm.cc
  )
  target_include_directories(lfif_tool_io PUBLIC include)
  target_link_libraries(lfif_tool_io PUBLIC ppm)
endif()

add_executable(lfif_cli src/lfif.cc)
set_target_properties(lfif_cli PROPERTIES OUTPUT_NAME lfif)
target_include_directories(lfif_cli PRIVATE include)
target_link_libraries(lfif_cli PRIVATE lfif_container ppm)
