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
  encoder.create(stream, image_size, block_size, 8, 0, false);
  encoder.encodeStream(pixels, stream);

  LFIFDecoder<4> decoder;
  decoder.open(stream);
  std::map<std::array<size_t, 4>, std::array<uint16_t, 3>> decoded;
  decoder.decodeStream(stream, [&](const auto &position, const auto &rgb) {
    decoded[position] = rgb;
  });

  ASSERT_EQ(decoded.size(), 16U);
  for (const auto &[position, rgb] : decoded) {
    EXPECT_EQ(rgb, pixels(position));
  }
}
