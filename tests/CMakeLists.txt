find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests
  test_bitstream.cc
  test_bwt.cc
  test_cabac.cc
  test_dct.cc
  test_huffman.cc
  test_lfif_header.cc
  test_lfif_roundtrip.cc
  test_lfwf_roundtrip.cc
  test_mtf.cc
  test_runlength.cc
  test_traversal_table.cc
  test_zigzag.cc
)
target_link_libraries(unit_tests PRIVATE lfif GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)
