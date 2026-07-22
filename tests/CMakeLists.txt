find_package(GTest CONFIG REQUIRED)

add_executable(
  lfif_unit_tests
  test_bitstream.cc
  test_block.cc
  test_bwt.cc
  test_cabac.cc
  test_codec_parameters.cc
  test_dct.cc
  test_dwt.cc
  test_format.cc
  test_huffman.cc
  test_lfif_roundtrip.cc
  test_lfwf_roundtrip.cc
  test_mtf.cc
  test_ppm.cc
  test_runlength.cc
  test_traversal_table.cc
  test_zigzag.cc
)
target_link_libraries(
  lfif_unit_tests
  PRIVATE lfif ppm GTest::gtest GTest::gtest_main lfif_project_options
)

include(GoogleTest)
gtest_discover_tests(lfif_unit_tests)

add_test(
  NAME ToolIo.CreatesOutputDirectoriesWithApostrophes
  COMMAND
    $<TARGET_FILE:lfif4d_compress>
    -i ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/one-pixel.ppm
    -o "${CMAKE_CURRENT_BINARY_DIR}/directory'with-apostrophe/output.lfif"
    -d 0
)

add_test(
  NAME ToolIo.RejectsNonSquareImageGrids
  COMMAND
    $<TARGET_FILE:lfif4d_compress>
    -i "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/non-square-#.ppm"
    -o ${CMAKE_CURRENT_BINARY_DIR}/non-square.lfif
    -d 0
)
set_tests_properties(ToolIo.RejectsNonSquareImageGrids PROPERTIES WILL_FAIL TRUE)
