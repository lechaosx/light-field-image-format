find_package(GTest REQUIRED)

file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cc")

add_executable(unit_tests ${SOURCES})
target_link_libraries(unit_tests PRIVATE lfif GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)
