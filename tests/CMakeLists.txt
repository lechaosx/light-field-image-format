find_package(GTest CONFIG REQUIRED)

add_executable(
  lfif_unit_tests
  test_bitstream.cc
  test_block.cc
  test_bwt.cc
  test_cabac.cc
  test_container_codec.cc
  test_dct.cc
  test_dwt.cc
  test_format.cc
  test_huffman.cc
  test_lfif_roundtrip.cc
  test_lfwf_roundtrip.cc
  test_mtf.cc
  test_ppm.cc
  test_prediction.cc
  test_runlength.cc
  test_traversal_table.cc
  test_zigzag.cc
)
target_link_libraries(
  lfif_unit_tests
  PRIVATE lfif lfif_container ppm GTest::gtest GTest::gtest_main
)
target_compile_definitions(
  lfif_unit_tests
  PRIVATE LFIF_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

include(GoogleTest)
gtest_discover_tests(lfif_unit_tests)

set(expect_failure ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/expect_failure.cmake --)

add_executable(create_test_lfif create_test_lfif.cc)
target_link_libraries(create_test_lfif PRIVATE lfif_container)

add_executable(compare_test_ppm compare_test_ppm.cc)
target_link_libraries(compare_test_ppm PRIVATE ppm)

add_test(
  NAME OmniCli.CreateMissingPayload
  COMMAND $<TARGET_FILE:create_test_lfif> ${CMAKE_CURRENT_BINARY_DIR}/missing-payload.lfif
)
set_tests_properties(
  OmniCli.CreateMissingPayload
  PROPERTIES FIXTURES_SETUP OmniCliMissingPayload
)
add_test(
  NAME OmniCli.RejectsMissingPayload
  COMMAND ${expect_failure} $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/missing-payload.lfif
)
set_tests_properties(
  OmniCli.RejectsMissingPayload
  PROPERTIES FIXTURES_REQUIRED OmniCliMissingPayload
)
add_test(
  NAME OmniCli.RejectsOutputWriteFailure
  COMMAND
    ${expect_failure}
    $<TARGET_FILE:lfif_cli>
    compress
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/one-pixel.ppm
    /dev/full
)
add_test(
  NAME OmniCli.RejectsNonBitDepthMaxval
  COMMAND
    ${expect_failure}
    $<TARGET_FILE:lfif_cli>
    compress
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/non-bit-depth-maxval.ppm
    ${CMAKE_CURRENT_BINARY_DIR}/invalid-maxval.lfif
)

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
  NAME OmniCli.WaveletInspectChannels
  COMMAND $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.lfif
)
set_tests_properties(
  OmniCli.WaveletInspectChannels
  PROPERTIES DEPENDS OmniCli.WaveletCompress PASS_REGULAR_EXPRESSION "channels: 3"
)
add_test(
  NAME OmniCli.WaveletInspectSampleFormat
  COMMAND $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.lfif
)
set_tests_properties(
  OmniCli.WaveletInspectSampleFormat
  PROPERTIES DEPENDS OmniCli.WaveletCompress PASS_REGULAR_EXPRESSION "sample format: unsigned integer"
)
add_test(
  NAME OmniCli.WaveletInspectEntropyCodec
  COMMAND $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.lfif
)
set_tests_properties(
  OmniCli.WaveletInspectEntropyCodec
  PROPERTIES DEPENDS OmniCli.WaveletCompress PASS_REGULAR_EXPRESSION "entropy codec: CABAC"
)
add_test(
  NAME OmniCli.WaveletInspectColorSpace
  COMMAND $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.lfif
)
set_tests_properties(
  OmniCli.WaveletInspectColorSpace
  PROPERTIES DEPENDS OmniCli.WaveletCompress PASS_REGULAR_EXPRESSION "color space: RGB"
)
add_test(
  NAME OmniCli.WaveletInspectDisparityCompensation
  COMMAND $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.lfif
)
set_tests_properties(
  OmniCli.WaveletInspectDisparityCompensation
  PROPERTIES DEPENDS OmniCli.WaveletCompress PASS_REGULAR_EXPRESSION "disparity compensation: no"
)
add_test(
  NAME OmniCli.WaveletInspectDisparityShifts
  COMMAND $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.lfif
)
set_tests_properties(
  OmniCli.WaveletInspectDisparityShifts
  PROPERTIES DEPENDS OmniCli.WaveletCompress PASS_REGULAR_EXPRESSION "disparity shifts: 0, 0"
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
  NAME OmniCli.WaveletOutputMatchesInput
  COMMAND
    $<TARGET_FILE:compare_test_ppm>
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/one-pixel.ppm
    ${CMAKE_CURRENT_BINARY_DIR}/omni-wavelet.ppm
)
set_tests_properties(
  OmniCli.WaveletOutputMatchesInput
  PROPERTIES DEPENDS OmniCli.WaveletDecompress
)

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
  NAME OmniCli.DctOutputMatchesInput
  COMMAND
    $<TARGET_FILE:compare_test_ppm>
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/one-pixel.ppm
    ${CMAKE_CURRENT_BINARY_DIR}/omni-dct.ppm
    --within-one
)
set_tests_properties(OmniCli.DctOutputMatchesInput PROPERTIES DEPENDS OmniCli.DctDecompress)

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
add_test(
  NAME OmniCli.FourDimensionalOutput00MatchesInput
  COMMAND
    $<TARGET_FILE:compare_test_ppm>
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/grid-00.ppm
    ${CMAKE_CURRENT_BINARY_DIR}/omni-4d-00.ppm
)
add_test(
  NAME OmniCli.FourDimensionalOutput01MatchesInput
  COMMAND
    $<TARGET_FILE:compare_test_ppm>
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/grid-01.ppm
    ${CMAKE_CURRENT_BINARY_DIR}/omni-4d-01.ppm
)
add_test(
  NAME OmniCli.FourDimensionalOutput02MatchesInput
  COMMAND
    $<TARGET_FILE:compare_test_ppm>
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/grid-02.ppm
    ${CMAKE_CURRENT_BINARY_DIR}/omni-4d-02.ppm
)
add_test(
  NAME OmniCli.FourDimensionalOutput03MatchesInput
  COMMAND
    $<TARGET_FILE:compare_test_ppm>
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/grid-03.ppm
    ${CMAKE_CURRENT_BINARY_DIR}/omni-4d-03.ppm
)
set_tests_properties(
  OmniCli.FourDimensionalOutput00MatchesInput
  OmniCli.FourDimensionalOutput01MatchesInput
  OmniCli.FourDimensionalOutput02MatchesInput
  OmniCli.FourDimensionalOutput03MatchesInput
  PROPERTIES DEPENDS OmniCli.FourDimensionalDecompress
)

add_test(
  NAME OmniCli.DisparityCompress
  COMMAND
    $<TARGET_FILE:lfif_cli>
    compress
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/grid-##.ppm"
    ${CMAKE_CURRENT_BINARY_DIR}/omni-disparity.lfif
    --shape 2x2
    --disparity 1x-1
)
add_test(
  NAME OmniCli.DisparityInspect
  COMMAND $<TARGET_FILE:lfif_cli> inspect ${CMAKE_CURRENT_BINARY_DIR}/omni-disparity.lfif
)
set_tests_properties(
  OmniCli.DisparityInspect
  PROPERTIES
    DEPENDS OmniCli.DisparityCompress
    PASS_REGULAR_EXPRESSION "disparity compensation: yes[\r\n]+disparity shifts: 1, -1"
)

if(LFIF_BUILD_EXTRAS)
  find_program(FFMPEG_EXECUTABLE NAMES ffmpeg REQUIRED)

  add_executable(create_extra_test_ppm create_extra_test_ppm.cc)
  target_link_libraries(create_extra_test_ppm PRIVATE ppm)

  add_test(
    NAME ExtraSmoke.CreateInput
    COMMAND $<TARGET_FILE:create_extra_test_ppm> ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
  )
  set_tests_properties(ExtraSmoke.CreateInput PROPERTIES FIXTURES_SETUP ExtraSmokeInput)

  add_test(
    NAME ExtraInput.CreateOversizedWidth
    COMMAND
      $<TARGET_FILE:create_extra_test_ppm>
      --oversized-tool-width
      ${CMAKE_CURRENT_BINARY_DIR}/oversized-width.ppm
  )
  set_tests_properties(
    ExtraInput.CreateOversizedWidth
    PROPERTIES FIXTURES_SETUP ExtraOversizedWidth
  )
  add_test(
    NAME ExtraInput.RejectsOversizedWidthPromptly
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:av1bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/oversized-width.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/oversized-width.txt
      -f 1 -l 1
  )
  set_tests_properties(
    ExtraInput.RejectsOversizedWidthPromptly
    PROPERTIES
      FIXTURES_REQUIRED ExtraOversizedWidth
      PASS_REGULAR_EXPRESSION "PPM dimensions exceed codec limits"
  )

  add_test(
    NAME ExtraIo.CreateDirectoryCandidate
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/ppm-directory
  )
  set_tests_properties(
    ExtraIo.CreateDirectoryCandidate
    PROPERTIES FIXTURES_SETUP ExtraIoDirectoryCandidate
  )
  add_test(
    NAME ExtraIo.ReportsDirectoryCandidate
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:av1bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/ppm-directory
      -o ${CMAKE_CURRENT_BINARY_DIR}/unreadable-input.txt
      -f 1 -l 1
  )
  set_tests_properties(
    ExtraIo.ReportsDirectoryCandidate
    PROPERTIES
      FIXTURES_REQUIRED ExtraIoDirectoryCandidate
      PASS_REGULAR_EXPRESSION "cannot map PPM"
  )

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
    NAME ExtraOutput.Av1BenchRejectsWriteFailure
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:av1bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o /dev/full
      -f 1 -l 1
  )
  add_test(
    NAME ExtraOutput.MozBenchRejectsWriteFailure
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:mozbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o /dev/full
      -f 75 -l 75 -s 1
  )
  add_test(
    NAME ExtraOutput.OpenJpegBenchRejectsWriteFailure
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:openjpegbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o /dev/full
      -f 40 -l 40 -s 1
  )
  add_test(
    NAME ExtraOutput.X265BenchRejectsWriteFailure
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:x265bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o /dev/full
      -f 1 -l 1
  )
  add_test(
    NAME ExtraOutput.XvcBenchRejectsWriteFailure
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:xvcbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o /dev/full
      -f 40 -l 40
  )
  set_tests_properties(
    ExtraOutput.Av1BenchRejectsWriteFailure
    ExtraOutput.MozBenchRejectsWriteFailure
    ExtraOutput.OpenJpegBenchRejectsWriteFailure
    ExtraOutput.X265BenchRejectsWriteFailure
    ExtraOutput.XvcBenchRejectsWriteFailure
    PROPERTIES FIXTURES_REQUIRED ExtraSmokeInput
  )

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
    NAME ExtraHevc.Create420Stream
    COMMAND
      ${FFMPEG_EXECUTABLE}
      -y -loglevel error
      -f lavfi -i color=c=red:size=64x64:rate=1
      -frames:v 1
      -pix_fmt yuv420p
      -c:v libx265
      -f hevc
      ${CMAKE_CURRENT_BINARY_DIR}/external-420.hevc
  )
  set_tests_properties(
    ExtraHevc.Create420Stream
    PROPERTIES FIXTURES_SETUP ExtraHevc420Stream
  )
  add_test(
    NAME ExtraHevc.Decompress420Stream
    COMMAND
      $<TARGET_FILE:hevc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/external-420.hevc
      -o "${CMAKE_CURRENT_BINARY_DIR}/hevc-420-#.ppm"
  )
  set_tests_properties(
    ExtraHevc.Decompress420Stream
    PROPERTIES
      FIXTURES_REQUIRED ExtraHevc420Stream
      FIXTURES_SETUP ExtraHevc420Output
  )
  add_test(
    NAME ExtraHevc.Check420Output
    COMMAND
      $<TARGET_FILE:create_extra_test_ppm>
      --check-red
      ${CMAKE_CURRENT_BINARY_DIR}/hevc-420-0.ppm
  )
  set_tests_properties(
    ExtraHevc.Check420Output
    PROPERTIES FIXTURES_REQUIRED ExtraHevc420Output
  )

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
    NAME ExtraXvc.CreateOversizedNal
    COMMAND
      $<TARGET_FILE:create_extra_test_ppm>
      --oversized-xvc-nal ${CMAKE_CURRENT_BINARY_DIR}/oversized-nal.xvc
  )
  set_tests_properties(
    ExtraXvc.CreateOversizedNal
    PROPERTIES FIXTURES_SETUP ExtraXvcOversizedNal
  )
  add_test(
    NAME ExtraXvc.RejectsEmptyInput
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:xvc_decompress>
      -i /dev/null
      -o "${CMAKE_CURRENT_BINARY_DIR}/empty-xvc-#.ppm"
  )
  add_test(
    NAME ExtraXvc.RejectsTruncatedNalSize
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:xvc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/truncated-size.xvc
      -o "${CMAKE_CURRENT_BINARY_DIR}/truncated-xvc-#.ppm"
  )
  set_tests_properties(
    ExtraXvc.RejectsTruncatedNalSize
    PROPERTIES FIXTURES_REQUIRED ExtraXvcTruncatedSize
  )
  add_test(
    NAME ExtraXvc.RejectsMalformedNal
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:xvc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/malformed.xvc
      -o "${CMAKE_CURRENT_BINARY_DIR}/malformed-xvc-#.ppm"
  )
  set_tests_properties(
    ExtraXvc.RejectsMalformedNal
    PROPERTIES FIXTURES_REQUIRED ExtraXvcMalformedInput
  )
  add_test(
    NAME ExtraXvc.RejectsOversizedNalBeforeAllocation
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:xvc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/oversized-nal.xvc
      -o "${CMAKE_CURRENT_BINARY_DIR}/oversized-xvc-#.ppm"
  )
  set_tests_properties(
    ExtraXvc.RejectsOversizedNalBeforeAllocation
    PROPERTIES FIXTURES_REQUIRED ExtraXvcOversizedNal
  )
  add_test(
    NAME ExtraXvc.RejectsOutputWriteFailure
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:xvc_compress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o /dev/full
      -q 20
  )
  set_tests_properties(
    ExtraXvc.RejectsOutputWriteFailure
    PROPERTIES FIXTURES_REQUIRED ExtraSmokeInput
  )
  add_test(
    NAME ExtraHevc.RejectsOutputWriteFailure
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:hevc_compress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o /dev/full
      -b 1000
  )
  set_tests_properties(
    ExtraHevc.RejectsOutputWriteFailure
    PROPERTIES FIXTURES_REQUIRED ExtraSmokeInput
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
      ${expect_failure}
      $<TARGET_FILE:hevc_decompress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/mask-input.hevc
      -o "${CMAKE_CURRENT_BINARY_DIR}/mask-output-#.ppm"
  )
  set_tests_properties(
    ExtraMask.RejectsInsufficientOutputMask
    PROPERTIES
      FIXTURES_REQUIRED ExtraMaskStream
  )

  add_test(
    NAME ExtraArgs.Av1RejectsDescendingBitrates
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:av1bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-av1.txt
      -f 2 -l 1
  )
  add_test(
    NAME ExtraArgs.X265RejectsDescendingBitrates
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:x265bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-x265.txt
      -f 2 -l 1
  )
  add_test(
    NAME ExtraArgs.Av1RejectsInt64BoundaryBitrate
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:av1bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-av1-boundary.txt
      -f 2251799813685248 -l 2251799813685248
  )
  add_test(
    NAME ExtraArgs.X265RejectsInt64BoundaryBitrate
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:x265bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-x265-boundary.txt
      -f 2251799813685248 -l 2251799813685248
  )
  add_test(
    NAME ExtraArgs.HevcRejectsInt64BoundaryBitrate
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:hevc_compress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-hevc-boundary.hevc
      -b 9223372036854775808
  )
  set_tests_properties(
    ExtraArgs.Av1RejectsInt64BoundaryBitrate
    ExtraArgs.X265RejectsInt64BoundaryBitrate
    ExtraArgs.HevcRejectsInt64BoundaryBitrate
    PROPERTIES
      FIXTURES_REQUIRED ExtraSmokeInput
      PASS_REGULAR_EXPRESSION "bitrate exceeds codec limits"
  )
  add_test(
    NAME ExtraArgs.MozBenchRejectsDescendingQualities
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:mozbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-mozjpeg.txt
      -f 80 -l 70
  )
  add_test(
    NAME ExtraArgs.OpenJpegRejectsDescendingPsnr
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:openjpegbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-openjpeg.txt
      -f 50 -l 40
  )
  add_test(
    NAME ExtraArgs.XvcRejectsDescendingQp
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:xvcbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-xvc.txt
      -f 40 -l 20
  )
  add_test(
    NAME ExtraArgs.XvcCompressRejectsInvalidQp
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:xvc_compress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-qp.xvc
      -q not-a-number
  )
  add_test(
    NAME ExtraArgs.Av1RejectsNumericSuffix
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:av1bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-av1-suffix.txt
      -f 1junk -l 1
  )
  add_test(
    NAME ExtraArgs.X265RejectsNumericSuffix
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:x265bench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-x265-suffix.txt
      -f 1junk -l 1
  )
  add_test(
    NAME ExtraArgs.MozBenchRejectsNumericSuffix
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:mozbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-mozjpeg-suffix.txt
      -f 75junk -l 75 -s 1
  )
  add_test(
    NAME ExtraArgs.OpenJpegRejectsNumericSuffix
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:openjpegbench>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-openjpeg-suffix.txt
      -f 40junk -l 40 -s 1
  )
  add_test(
    NAME ExtraArgs.HevcCompressRejectsNumericSuffix
    COMMAND
      ${expect_failure}
      $<TARGET_FILE:hevc_compress>
      -i ${CMAKE_CURRENT_BINARY_DIR}/extra-input.ppm
      -o ${CMAKE_CURRENT_BINARY_DIR}/invalid-hevc-suffix.hevc
      -b 1000junk
  )
  set_tests_properties(
    ExtraArgs.Av1RejectsDescendingBitrates
    ExtraArgs.X265RejectsDescendingBitrates
    ExtraArgs.MozBenchRejectsDescendingQualities
    ExtraArgs.OpenJpegRejectsDescendingPsnr
    ExtraArgs.XvcRejectsDescendingQp
    ExtraArgs.XvcCompressRejectsInvalidQp
    ExtraArgs.Av1RejectsNumericSuffix
    ExtraArgs.X265RejectsNumericSuffix
    ExtraArgs.MozBenchRejectsNumericSuffix
    ExtraArgs.OpenJpegRejectsNumericSuffix
    ExtraArgs.HevcCompressRejectsNumericSuffix
    PROPERTIES FIXTURES_REQUIRED ExtraSmokeInput
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
