add_library(ppm STATIC src/ppm.cc)

target_include_directories(
  ppm
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(ppm PRIVATE lfif_project_options)
target_compile_features(ppm PUBLIC cxx_std_20)
