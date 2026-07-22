find_package(GTest CONFIG REQUIRED)

add_executable(
  lfif_unit_tests
  test_bitstream.cc
  test_block.cc
  test_bwt.cc
  test_cabac.cc
  test_codec_parameters.cc
  test_container_codec.cc
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
target_compile_definitions(
  lfif_unit_tests
  PRIVATE LFIF_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

include(GoogleTest)
gtest_discover_tests(lfif_unit_tests)

add_test(
  NAME OmniCli.CreatesOutputDirectoriesWithApostrophes
  COMMAND
    $<TARGET_FILE:lfif_cli>
    compress
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/one-pixel.ppm
    "${CMAKE_CURRENT_BINARY_DIR}/directory'with-apostrophe/output.lfif"
)

add_test(
  NAME OmniCli.WaveletCompress
  COMMAND
    $<TARGET_FILE:lfif_cli>
    compress
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/one-pixel.ppm
    ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.lfif
)
add_test(
  NAME OmniCli.WaveletInspect
  COMMAND $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.lfif
)
set_tests_properties(
  OmniCli.WaveletInspect
  PROPERTIES DEPENDS OmniCli.WaveletCompress PASS_REGULAR_EXPRESSION "transform: wavelet"
)
add_test(
  NAME OmniCli.WaveletDecompress
  COMMAND
    $<TARGET_FILE:lfif_cli>
    decompress
    ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.lfif
    ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.ppm
)
set_tests_properties(OmniCli.WaveletDecompress PROPERTIES DEPENDS OmniCli.WaveletCompress)

add_test(
  NAME OmniCli.DctCompress
  COMMAND
    $<TARGET_FILE:lfif_cli>
    compress
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/one-pixel.ppm
    ${CMAKE_CURRENT_BINARY_DIR}/omni-dct.lfif
    --transform dct
)
add_test(
  NAME OmniCli.DctInspect
  COMMAND $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/omni-dct.lfif
)
set_tests_properties(
  OmniCli.DctInspect
  PROPERTIES DEPENDS OmniCli.DctCompress PASS_REGULAR_EXPRESSION "transform: dct"
)
add_test(
  NAME OmniCli.DctDecompress
  COMMAND
    $<TARGET_FILE:lfif_cli>
    decompress
    ${CMAKE_CURRENT_BINARY_DIR}/omni-dct.lfif
    ${CMAKE_CURRENT_BINARY_DIR}/omni-dct.ppm
)
set_tests_properties(OmniCli.DctDecompress PROPERTIES DEPENDS OmniCli.DctCompress)

add_test(
  NAME OmniCli.FourDimensionalCompress
  COMMAND
    $<TARGET_FILE:lfif_cli>
    compress
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/grid-##.ppm"
    ${CMAKE_CURRENT_BINARY_DIR}/omni-4d.lfif
    --shape 2x2
)
add_test(
  NAME OmniCli.FourDimensionalInspect
  COMMAND $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/omni-4d.lfif
)
set_tests_properties(
  OmniCli.FourDimensionalInspect
  PROPERTIES DEPENDS OmniCli.FourDimensionalCompress PASS_REGULAR_EXPRESSION "dimensions: 4"
)
add_test(
  NAME OmniCli.FourDimensionalDecompress
  COMMAND
    $<TARGET_FILE:lfif_cli>
    decompress
    ${CMAKE_CURRENT_BINARY_DIR}/omni-4d.lfif
    "${CMAKE_CURRENT_BINARY_DIR}/omni-4d-##.ppm"
)
set_tests_properties(
  OmniCli.FourDimensionalDecompress
  PROPERTIES DEPENDS OmniCli.FourDimensionalCompress
)

if(LFIF_BUILD_EXTRAS)
  add_executable(create_extra_test_ppm create_extra_test_ppm.cc)
  target_link_libraries(create_extra_test_ppm PRIVATE ppm lfif_project_options)

  add_test(
    NAME ExtraSmoke.CreateInput
    COMMAND $<TARGET_FILE:create_extra_test_ppm> ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
  )
  set_tests_properties(ExtraSmoke.CreateInput PROPERTIES FIXTURES_SETUP ExtraSmokeInput)

  add_test(
    NAME ExtraSmoke.Av1Bench
    COMMAND
      $<TARGET_FILE:av1bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/av1bench.txt
      -f 1 -l 1
  )
  add_test(
    NAME ExtraSmoke.MozBench
    COMMAND
      $<TARGET_FILE:mozbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/mozbench.txt
      -f 75 -l 75 -s 1
  )
  add_test(
    NAME ExtraSmoke.OpenJpegBench
    COMMAND
      $<TARGET_FILE:openjpegbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/openjpegbench.txt
      -f 40 -l 40 -s 1
  )
  set_tests_properties(ExtraSmoke.OpenJpegBench PROPERTIES PASS_REGULAR_EXPRESSION "40 [0-9]")
  add_test(
    NAME ExtraSmoke.OpenJpegTinyImage
    COMMAND
      $<TARGET_FILE:openjpegbench>
      -i ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/one-pixel.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/openjpeg-tiny.txt
      -f 40 -l 40 -s 1
  )
  set_tests_properties(ExtraSmoke.OpenJpegTinyImage PROPERTIES PASS_REGULAR_EXPRESSION "40 (inf|[0-9])")
  add_test(
    NAME ExtraSmoke.X265Bench
    COMMAND
      $<TARGET_FILE:x265bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/x265bench.txt
      -f 1 -l 1
  )
  add_test(
    NAME ExtraSmoke.XvcBench
    COMMAND
      $<TARGET_FILE:xvcbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/xvcbench.txt
      -f 40 -l 40
  )
  set_tests_properties(ExtraSmoke.XvcBench PROPERTIES PASS_REGULAR_EXPRESSION "40 [0-9]")

  add_test(
    NAME ExtraSmoke.HevcCompress
    COMMAND
      $<TARGET_FILE:hevc_compress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/extra.hevc
      -b 1000
  )
  add_test(
    NAME ExtraSmoke.HevcDecompress
    COMMAND
      $<TARGET_FILE:hevc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra.hevc
      -o "${CMAKE_CURRENT_BINARY_DIR}/hevc-#.ppm"
  )
  set_tests_properties(ExtraSmoke.HevcDecompress PROPERTIES DEPENDS ExtraSmoke.HevcCompress)
  add_test(
    NAME ExtraSmoke.HevcOutput
    COMMAND
      $<TARGET_FILE:create_extra_test_ppm>
      --check ${CMAKE_CURRENT_BINARY_DIR}/hevc-0.ppm
  )
  set_tests_properties(ExtraSmoke.HevcOutput PROPERTIES DEPENDS ExtraSmoke.HevcDecompress)

  add_test(
    NAME ExtraSmoke.XvcCompress
    COMMAND
      $<TARGET_FILE:xvc_compress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/extra.xvc
      -q 20
  )
  add_test(
    NAME ExtraSmoke.XvcDecompress
    COMMAND
      $<TARGET_FILE:xvc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra.xvc
      -o "${CMAKE_CURRENT_BINARY_DIR}/xvc-#.ppm"
  )
  set_tests_properties(ExtraSmoke.XvcDecompress PROPERTIES DEPENDS ExtraSmoke.XvcCompress)
  add_test(
    NAME ExtraSmoke.XvcOutput
    COMMAND
      $<TARGET_FILE:create_extra_test_ppm>
      --check ${CMAKE_CURRENT_BINARY_DIR}/xvc-0.ppm
  )
  set_tests_properties(ExtraSmoke.XvcOutput PROPERTIES DEPENDS ExtraSmoke.XvcDecompress)

  add_test(
    NAME ExtraXvc.MultiCompress
    COMMAND
      $<TARGET_FILE:xvc_compress>
      -i "${CMAKE_CURRENT_BINARY_DIR}/mask-input-##.ppm"
      -o ${CMAKE_CURRENT_BINARY_DIR}/multi.xvc
      -q 20
  )
  set_tests_properties(
    ExtraXvc.MultiCompress
    PROPERTIES FIXTURES_REQUIRED ExtraMaskInput FIXTURES_SETUP ExtraXvcStream
  )
  add_test(
    NAME ExtraXvc.MultiDecompress
    COMMAND
      $<TARGET_FILE:xvc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/multi.xvc
      -o "${CMAKE_CURRENT_BINARY_DIR}/xvc-multi-##.ppm"
  )
  set_tests_properties(
    ExtraXvc.MultiDecompress
    PROPERTIES FIXTURES_REQUIRED ExtraXvcStream FIXTURES_SETUP ExtraXvcOutput
  )
  add_test(
    NAME ExtraXvc.MultiOutput
    COMMAND
      $<TARGET_FILE:create_extra_test_ppm>
      --check-grid ${CMAKE_CURRENT_BINARY_DIR}/xvc-multi- 16
  )
  set_tests_properties(ExtraXvc.MultiOutput PROPERTIES FIXTURES_REQUIRED ExtraXvcOutput)

  add_test(
    NAME ExtraXvc.CreateTruncatedSize
    COMMAND
      $<TARGET_FILE:create_extra_test_ppm>
      --truncated-xvc-size ${CMAKE_CURRENT_BINARY_DIR}/truncated-size.xvc
  )
  set_tests_properties(
    ExtraXvc.CreateTruncatedSize
    PROPERTIES FIXTURES_SETUP ExtraXvcTruncatedSize
  )
  add_test(
    NAME ExtraXvc.CreateMalformedInput
    COMMAND
      $<TARGET_FILE:create_extra_test_ppm>
      --malformed-xvc ${CMAKE_CURRENT_BINARY_DIR}/malformed.xvc
  )
  set_tests_properties(
    ExtraXvc.CreateMalformedInput
    PROPERTIES FIXTURES_SETUP ExtraXvcMalformedInput
  )
  add_test(
    NAME ExtraXvc.RejectsEmptyInput
    COMMAND
      $<TARGET_FILE:xvc_decompress>
      -i /dev/null
      -o "${CMAKE_CURRENT_BINARY_DIR}/empty-xvc-#.ppm"
  )
  set_tests_properties(ExtraXvc.RejectsEmptyInput PROPERTIES WILL_FAIL TRUE)
  add_test(
    NAME ExtraXvc.RejectsTruncatedNalSize
    COMMAND
      $<TARGET_FILE:xvc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/truncated-size.xvc
      -o "${CMAKE_CURRENT_BINARY_DIR}/truncated-xvc-#.ppm"
  )
  set_tests_properties(
    ExtraXvc.RejectsTruncatedNalSize
    PROPERTIES FIXTURES_REQUIRED ExtraXvcTruncatedSize WILL_FAIL TRUE
  )
  add_test(
    NAME ExtraXvc.RejectsMalformedNal
    COMMAND
      $<TARGET_FILE:xvc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/malformed.xvc
      -o "${CMAKE_CURRENT_BINARY_DIR}/malformed-xvc-#.ppm"
  )
  set_tests_properties(
    ExtraXvc.RejectsMalformedNal
    PROPERTIES FIXTURES_REQUIRED ExtraXvcMalformedInput WILL_FAIL TRUE
  )
  add_test(
    NAME ExtraXvc.RejectsOutputWriteFailure
    COMMAND
      $<TARGET_FILE:xvc_compress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o /dev/full
      -q 20
  )
  set_tests_properties(
    ExtraXvc.RejectsOutputWriteFailure
    PROPERTIES FIXTURES_REQUIRED ExtraSmokeInput WILL_FAIL TRUE
  )

  add_test(
    NAME ExtraMask.CreateInput
    COMMAND
      $<TARGET_FILE:create_extra_test_ppm>
      --grid ${CMAKE_CURRENT_BINARY_DIR}/mask-input- 16
  )
  set_tests_properties(ExtraMask.CreateInput PROPERTIES FIXTURES_SETUP ExtraMaskInput)
  add_test(
    NAME ExtraMask.HevcCompress
    COMMAND
      $<TARGET_FILE:hevc_compress>
      -i "${CMAKE_CURRENT_BINARY_DIR}/mask-input-##.ppm"
      -o ${CMAKE_CURRENT_BINARY_DIR}/mask-input.hevc
      -b 1000
  )
  set_tests_properties(
    ExtraMask.HevcCompress
    PROPERTIES FIXTURES_REQUIRED ExtraMaskInput FIXTURES_SETUP ExtraMaskStream
  )
  add_test(
    NAME ExtraMask.RejectsInsufficientOutputMask
    COMMAND
      $<TARGET_FILE:hevc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/mask-input.hevc
      -o "${CMAKE_CURRENT_BINARY_DIR}/mask-output-#.ppm"
  )
  set_tests_properties(
    ExtraMask.RejectsInsufficientOutputMask
    PROPERTIES
      FIXTURES_REQUIRED ExtraMaskStream
      WILL_FAIL TRUE
  )

  add_test(
    NAME ExtraArgs.Av1RejectsDescendingBitrates
    COMMAND
      $<TARGET_FILE:av1bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-av1.txt
      -f 2 -l 1
  )
  add_test(
    NAME ExtraArgs.X265RejectsDescendingBitrates
    COMMAND
      $<TARGET_FILE:x265bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-x265.txt
      -f 2 -l 1
  )
  add_test(
    NAME ExtraArgs.MozBenchRejectsDescendingQualities
    COMMAND
      $<TARGET_FILE:mozbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-mozjpeg.txt
      -f 80 -l 70
  )
  add_test(
    NAME ExtraArgs.OpenJpegRejectsDescendingPsnr
    COMMAND
      $<TARGET_FILE:openjpegbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-openjpeg.txt
      -f 50 -l 40
  )
  add_test(
    NAME ExtraArgs.XvcRejectsDescendingQp
    COMMAND
      $<TARGET_FILE:xvcbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-xvc.txt
      -f 40 -l 20
  )
  set_tests_properties(
    ExtraArgs.Av1RejectsDescendingBitrates
    ExtraArgs.X265RejectsDescendingBitrates
    ExtraArgs.MozBenchRejectsDescendingQualities
    ExtraArgs.OpenJpegRejectsDescendingPsnr
    ExtraArgs.XvcRejectsDescendingQp
    PROPERTIES FIXTURES_REQUIRED ExtraSmokeInput WILL_FAIL TRUE
  )

  set_tests_properties(
    ExtraSmoke.Av1Bench
    ExtraSmoke.MozBench
    ExtraSmoke.OpenJpegBench
    ExtraSmoke.X265Bench
    ExtraSmoke.XvcBench
    ExtraSmoke.HevcCompress
    ExtraSmoke.HevcDecompress
    ExtraSmoke.HevcOutput
    ExtraSmoke.XvcCompress
    ExtraSmoke.XvcDecompress
    ExtraSmoke.XvcOutput
    PROPERTIES FIXTURES_REQUIRED ExtraSmokeInput
  )
endif()
