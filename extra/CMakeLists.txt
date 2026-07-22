find_package(PkgConfig REQUIRED)

pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)
pkg_check_modules(OPENJPEG REQUIRED IMPORTED_TARGET libopenjp2)
pkg_check_modules(XVC REQUIRED IMPORTED_TARGET xvc)

set(MOZJPEG_ROOT "$ENV{MOZJPEG_ROOT}" CACHE PATH "Mozilla mozjpeg prefix")
find_path(
  MOZJPEG_INCLUDE_DIR
  NAMES jpeglib.h
  PATHS "${MOZJPEG_ROOT}/include"
  NO_DEFAULT_PATH
  REQUIRED
)
find_library(
  MOZJPEG_LIBRARY
  NAMES jpeg
  PATHS "${MOZJPEG_ROOT}/lib"
  NO_DEFAULT_PATH
  REQUIRED
)

add_library(mozjpeg UNKNOWN IMPORTED)
set_target_properties(
  mozjpeg
  PROPERTIES
    IMPORTED_LOCATION "${MOZJPEG_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${MOZJPEG_INCLUDE_DIR}"
)

add_executable(av1bench av1bench.cc)
target_link_libraries(
  av1bench
  PRIVATE
    lfif_tool_io
    PkgConfig::AVCODEC
    PkgConfig::AVUTIL
    PkgConfig::SWSCALE
    lfif_project_options
)

add_executable(hevc_compress hevc_compress.cc)
target_link_libraries(
  hevc_compress
  PRIVATE
    lfif_tool_io
    PkgConfig::AVCODEC
    PkgConfig::AVUTIL
    PkgConfig::SWSCALE
    lfif_project_options
)

add_executable(hevc_decompress hevc_decompress.cc)
target_link_libraries(
  hevc_decompress
  PRIVATE
    lfif_tool_io
    PkgConfig::AVCODEC
    PkgConfig::AVUTIL
    PkgConfig::SWSCALE
    lfif_project_options
)

add_executable(mozbench mozbench.cc)
target_link_libraries(
  mozbench
  PRIVATE lfif_tool_io mozjpeg lfif_project_options
)

add_executable(openjpegbench openjpegbench.cc)
target_link_libraries(
  openjpegbench
  PRIVATE lfif_tool_io PkgConfig::OPENJPEG lfif_project_options
)

add_executable(x265bench x265bench.cc)
target_link_libraries(
  x265bench
  PRIVATE
    lfif_tool_io
    PkgConfig::AVCODEC
    PkgConfig::AVUTIL
    PkgConfig::SWSCALE
    lfif_project_options
)

add_executable(xvc_compress xvc_compress.cc)
target_link_libraries(
  xvc_compress
  PRIVATE
    lfif_tool_io
    PkgConfig::SWSCALE
    PkgConfig::XVC
    lfif_project_options
)

add_executable(xvc_decompress xvc_decompress.cc)
target_link_libraries(
  xvc_decompress
  PRIVATE
    lfif_tool_io
    PkgConfig::SWSCALE
    PkgConfig::XVC
    lfif_project_options
)

add_executable(xvcbench xvcbench.cc)
target_link_libraries(
  xvcbench
  PRIVATE lfif_tool_io PkgConfig::XVC lfif_project_options
)

install(
  TARGETS
    av1bench
    hevc_compress
    hevc_decompress
    mozbench
    openjpegbench
    x265bench
    xvc_compress
    xvc_decompress
    xvcbench
  RUNTIME DESTINATION bin
)
