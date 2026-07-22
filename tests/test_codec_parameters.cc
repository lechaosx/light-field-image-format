#include <gtest/gtest.h>

#include <components/endian.h>
#include <lfif_decoder.h>
#include <lfif_encoder.h>
#include <lfwf_decoder.h>
#include <lfwf_encoder.h>

#include <array>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

template <size_t D>
std::stringstream header(
    const std::array<size_t, D> &size,
    const std::array<size_t, D> &block_size,
    uint8_t depth_bits) {
  std::stringstream stream;
  writeValueToStream<uint8_t>(depth_bits, stream);
  writeValueToStream<uint8_t>(0, stream);
  writeValueToStream<bool>(false, stream);
  for (const size_t extent : size) {
    writeValueToStream<uint64_t>(extent, stream);
  }
  for (const size_t extent : block_size) {
    writeValueToStream<uint64_t>(extent, stream);
  }
  return stream;
}

} // namespace

TEST(CodecParameters, EncodersRejectZeroImageExtentsBeforeWriting) {
  std::stringstream dct_stream;
  LFIFEncoder<2> dct_encoder;
  EXPECT_THROW(dct_encoder.create(dct_stream, {0, 4}, {2, 2}, 8, 0, false), std::invalid_argument);
  EXPECT_TRUE(dct_stream.str().empty());

  std::stringstream wavelet_stream;
  LFWFEncoder<2> wavelet_encoder;
  EXPECT_THROW(wavelet_encoder.create(wavelet_stream, {4, 0}, {2, 2}, 8, 0, false), std::invalid_argument);
  EXPECT_TRUE(wavelet_stream.str().empty());
}

TEST(CodecParameters, EncodersRejectZeroBlockExtentsBeforeWriting) {
  std::stringstream dct_stream;
  LFIFEncoder<2> dct_encoder;
  EXPECT_THROW(dct_encoder.create(dct_stream, {4, 4}, {0, 2}, 8, 0, false), std::invalid_argument);
  EXPECT_TRUE(dct_stream.str().empty());

  std::stringstream wavelet_stream;
  LFWFEncoder<2> wavelet_encoder;
  EXPECT_THROW(wavelet_encoder.create(wavelet_stream, {4, 4}, {2, 0}, 8, 0, false), std::invalid_argument);
  EXPECT_TRUE(wavelet_stream.str().empty());
}

TEST(CodecParameters, EncodersRejectUnsupportedSampleDepths) {
  std::stringstream stream;
  LFWFEncoder<2> encoder;
  EXPECT_THROW(encoder.create(stream, {4, 4}, {2, 2}, 0, 0, false), std::invalid_argument);
  EXPECT_THROW(encoder.create(stream, {4, 4}, {2, 2}, 17, 0, false), std::invalid_argument);
}

TEST(CodecParameters, DecodersRejectInvalidHeadersWhenOpened) {
  auto dct_stream = header<2>({4, 4}, {0, 2}, 8);
  LFIFDecoder<2> dct_decoder;
  EXPECT_THROW(dct_decoder.open(dct_stream), std::invalid_argument);

  auto wavelet_stream = header<2>({4, 4}, {2, 2}, 17);
  LFWFDecoder<2> wavelet_decoder;
  EXPECT_THROW(wavelet_decoder.open(wavelet_stream), std::invalid_argument);
}

TEST(CodecParameters, EncodersRejectAlignmentOverflow) {
  std::stringstream stream;
  LFWFEncoder<2> encoder;
  EXPECT_THROW(
      encoder.create(stream, {std::numeric_limits<size_t>::max(), 1}, {2, 1}, 8, 0, false),
      std::length_error);
  EXPECT_THROW(
      encoder.create(stream, {std::numeric_limits<size_t>::max() / 2 + 1, 2}, {1, 1}, 8, 0, false),
      std::length_error);
}
