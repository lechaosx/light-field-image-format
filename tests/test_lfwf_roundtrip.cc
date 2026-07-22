#include <gtest/gtest.h>

#include <lfwf_encoder.h>
#include <lfwf_decoder.h>

#include <array>
#include <cstdint>
#include <map>
#include <sstream>

namespace {

constexpr uint8_t depth_bits = 8;

std::array<uint16_t, 3> synthetic_2d(const std::array<size_t, 2> &pos) {
  const uint16_t v = static_cast<uint16_t>((pos[0] * 7 + pos[1] * 13) % 256);
  return {v, static_cast<uint16_t>(255 - v), static_cast<uint16_t>(pos[0] % 256)};
}

std::array<uint16_t, 3> synthetic_4d(const std::array<size_t, 4> &pos) {
  const uint16_t v = static_cast<uint16_t>((pos[0] * 7 + pos[1] * 13 + pos[2] * 29 + pos[3] * 53) % 256);
  return {v, static_cast<uint16_t>(255 - v), static_cast<uint16_t>((pos[2] + pos[3]) % 256)};
}

} // namespace

TEST(LfwfRoundTrip, ReconstructsEveryPixelExactly2D) {
  const std::array<uint64_t, 2> image_size {13, 10};
  const std::array<uint64_t, 2> block_size {8, 8};

  std::stringstream stream;
  LFWFEncoder<2> encoder {};
  encoder.create(stream, image_size, block_size, depth_bits, /*distortion*/ 0, /*predict*/ false);
  encoder.encodeStream(synthetic_2d, stream);

  LFWFDecoder<2> decoder {};
  decoder.open(stream);

  std::map<std::array<size_t, 2>, std::array<uint16_t, 3>> decoded;
  decoder.decodeStream(stream, [&](const std::array<size_t, 2> &pos, const std::array<uint16_t, 3> &rgb) {
    decoded[pos] = rgb;
  });

  ASSERT_EQ(decoded.size(), image_size[0] * image_size[1]);
  for (const auto &[pos, rgb] : decoded) {
    EXPECT_EQ(rgb, synthetic_2d(pos)) << "at (" << pos[0] << ", " << pos[1] << ")";
  }
}

TEST(LfwfRoundTrip, ReconstructsEveryPixelExactly4D) {
  const std::array<uint64_t, 4> image_size {6, 5, 3, 3};
  const std::array<uint64_t, 4> block_size {4, 4, 2, 2};

  std::stringstream stream;
  LFWFEncoder<4> encoder {};
  encoder.create(stream, image_size, block_size, depth_bits, /*distortion*/ 0, /*predict*/ false);
  encoder.encodeStream(synthetic_4d, stream);

  LFWFDecoder<4> decoder {};
  decoder.open(stream);

  std::map<std::array<size_t, 4>, std::array<uint16_t, 3>> decoded;
  decoder.decodeStream(stream, [&](const std::array<size_t, 4> &pos, const std::array<uint16_t, 3> &rgb) {
    decoded[pos] = rgb;
  });

  ASSERT_EQ(decoded.size(), image_size[0] * image_size[1] * image_size[2] * image_size[3]);
  for (const auto &[pos, rgb] : decoded) {
    EXPECT_EQ(rgb, synthetic_4d(pos))
        << "at (" << pos[0] << ", " << pos[1] << ", " << pos[2] << ", " << pos[3] << ")";
  }
}

TEST(LfwfRoundTrip, CompressionIsDeterministic) {
  const std::array<uint64_t, 2> image_size {24, 16};
  const std::array<uint64_t, 2> block_size {8, 8};

  auto compress = [&] {
    std::stringstream stream;
    LFWFEncoder<2> encoder {};
    encoder.create(stream, image_size, block_size, depth_bits, 0, false);
    encoder.encodeStream(synthetic_2d, stream);
    return stream.str();
  };

  EXPECT_EQ(compress(), compress());
}

TEST(LfwfRoundTrip, ReconstructsEveryPixelExactlyWithPrediction) {
  const std::array<uint64_t, 2> image_size {17, 9};
  const std::array<uint64_t, 2> block_size {8, 8};

  std::stringstream stream;
  LFWFEncoder<2> encoder {};
  encoder.create(stream, image_size, block_size, depth_bits, 0, true);
  encoder.encodeStream(synthetic_2d, stream);

  LFWFDecoder<2> decoder {};
  decoder.open(stream);
  std::map<std::array<size_t, 2>, std::array<uint16_t, 3>> decoded;
  decoder.decodeStream(stream, [&](const auto &pos, const auto &rgb) { decoded[pos] = rgb; });

  ASSERT_EQ(decoded.size(), image_size[0] * image_size[1]);
  for (const auto &[pos, rgb] : decoded) {
    EXPECT_EQ(rgb, synthetic_2d(pos));
  }
}
