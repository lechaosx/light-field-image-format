file(GLOB SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)

add_library(lfif STATIC ${SOURCES})
target_include_directories(lfif PUBLIC include)
target_compile_features(lfif PUBLIC cxx_std_23)
target_link_libraries(lfif PRIVATE OpenMP::OpenMP_CXX)
