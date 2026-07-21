#include <gtest/gtest.h>

#include <lfwf_encoder.h>
#include <lfwf_decoder.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr uint64_t width = 24;
constexpr uint64_t height = 16;
constexpr uint8_t depth_bits = 8;

std::array<uint16_t, 3> synthetic_pixel(const std::array<size_t, 2> &pos) {
  const uint16_t v = static_cast<uint16_t>((pos[0] * 7 + pos[1] * 13) % 256);
  return {v, static_cast<uint16_t>(255 - v), static_cast<uint16_t>(pos[0] % 256)};
}

std::string compress(const std::array<uint64_t, 2> &image_size,
                     const std::array<uint64_t, 2> &block_size) {
  std::stringstream stream;
  stream << "LFIF-2D\n";
  LFWFEncoder<2> encoder {};
  encoder.create(stream, image_size, block_size, depth_bits, /*distortion*/ 0, /*predict*/ false);
  encoder.encodeStream(synthetic_pixel, stream);
  return stream.str();
}

} // namespace

TEST(LfwfRoundTrip, HeaderAndGeometrySurviveEncodeDecode) {
  const std::array<uint64_t, 2> image_size {width, height};
  const std::array<uint64_t, 2> block_size {8, 8};

  std::stringstream stream(compress(image_size, block_size));

  std::string magic;
  stream >> magic;
  stream.ignore();

  LFWFDecoder<2> decoder {};
  decoder.open(stream);

  std::vector<bool> visited(width * height, false);
  size_t visited_count = 0;
  auto pusher = [&](const std::array<size_t, 2> &pos, const std::array<uint16_t, 3> &) {
    visited[pos[1] * width + pos[0]] = true;
    visited_count++;
  };
  decoder.decodeStream(stream, pusher);

  EXPECT_EQ(magic, "LFIF-2D");
  EXPECT_EQ(decoder.header.size[0], width);
  EXPECT_EQ(decoder.header.size[1], height);
  EXPECT_EQ(decoder.header.depth_bits, depth_bits);
  EXPECT_EQ(visited_count, width * height);
  EXPECT_EQ(std::count(visited.begin(), visited.end(), true), static_cast<long>(width * height));
}

TEST(LfwfRoundTrip, CompressionIsDeterministic) {
  const std::array<uint64_t, 2> image_size {width, height};
  const std::array<uint64_t, 2> block_size {8, 8};

  EXPECT_EQ(compress(image_size, block_size), compress(image_size, block_size));
}
