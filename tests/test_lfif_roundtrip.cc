#include <gtest/gtest.h>

#include <lfif_decoder.h>
#include <lfif_encoder.h>

#include <array>
#include <cstdint>
#include <map>
#include <sstream>

namespace {

std::array<uint16_t, 3> constant_gray_4d(const std::array<size_t, 4> &) {
  return {64, 64, 64};
}

} // namespace

TEST(LfifRoundTrip, ReconstructsConstantGray4D) {
  const std::array<uint64_t, 4> image_size {2, 2, 2, 2};
  const std::array<uint64_t, 4> block_size {2, 2, 2, 2};

  std::stringstream stream;
  LFIFEncoder<4> encoder {};
  encoder.create(stream, image_size, block_size, 8, 0, false);
  encoder.encodeStream(constant_gray_4d, stream);

  LFIFDecoder<4> decoder {};
  decoder.open(stream);
  std::map<std::array<size_t, 4>, std::array<uint16_t, 3>> decoded;
  decoder.decodeStream(stream, [&](const auto &pos, const auto &rgb) { decoded[pos] = rgb; });

  ASSERT_EQ(decoded.size(), 16u);
  for (const auto &[pos, rgb] : decoded) {
    EXPECT_EQ(rgb, constant_gray_4d(pos));
  }
}
