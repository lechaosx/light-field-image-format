#include <gtest/gtest.h>

#include <components/endian.h>
#include <lfif_decoder.h>
#include <lfwf_decoder.h>

#include <array>
#include <cstdint>
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

}

TEST(CodecParameters, DecodersRejectInvalidHeadersWhenOpened) {
  auto dct_stream = header<2>({4, 4}, {0, 2}, 8);
  LFIFDecoder<2> dct_decoder;
  EXPECT_THROW(dct_decoder.open(dct_stream), std::invalid_argument);

  auto wavelet_stream = header<2>({4, 4}, {2, 2}, 17);
  LFWFDecoder<2> wavelet_decoder;
  EXPECT_THROW(wavelet_decoder.open(wavelet_stream), std::invalid_argument);
}
