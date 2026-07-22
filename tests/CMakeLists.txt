find_package(GTest CONFIG REQUIRED)

add_executable(
  lfif_unit_tests
  test_bitstream.cc
  test_bwt.cc
  test_cabac.cc
  test_dct.cc
  test_dwt.cc
  test_huffman.cc
  test_mtf.cc
  test_runlength.cc
  test_traversal_table.cc
  test_zigzag.cc
)
target_link_libraries(
  lfif_unit_tests
  PRIVATE lfif GTest::gtest GTest::gtest_main lfif_project_options
)

include(GoogleTest)
gtest_discover_tests(lfif_unit_tests)
