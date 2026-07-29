#include <gtest/gtest.h>

#include <lfif_decoder.h>
#include <lfif_encoder.h>

#include <array>
#include <cstdint>
#include <map>
#include <sstream>

TEST(LfifRoundTrip, ReconstructsConstantFourDimensionalImage) {
  const std::array<size_t, 4> image_size {2, 2, 2, 2};
  const std::array<size_t, 4> block_size {2, 2, 2, 2};
  const auto pixels = [](const std::array<size_t, 4> &) {
    return std::array<uint16_t, 3> {64, 64, 64};
  };

  std::stringstream stream;
  LFIFEncoder<4> encoder;
  encoder.size = image_size;
  encoder.block_size = block_size;
  encoder.depth_bits = 8;
  encoder.discarded_bits = 0;
  encoder.predicted = false;
  encoder.encodeStream(pixels, stream);

  LFIFDecoder<4> decoder;
  decoder.size = image_size;
  decoder.block_size = block_size;
  decoder.depth_bits = 8;
  decoder.discarded_bits = 0;
  decoder.predicted = false;
  std::map<std::array<size_t, 4>, std::array<uint16_t, 3>> decoded;
  decoder.decodeStream(stream, [&](const auto &position, const auto &rgb) {
    decoded[position] = rgb;
  });

  ASSERT_EQ(decoded.size(), 16U);
  for (const auto &[position, rgb] : decoded) {
    EXPECT_EQ(rgb, pixels(position));
  }
}
