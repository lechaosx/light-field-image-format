#include <gtest/gtest.h>

#include <lfwf_decoder.h>
#include <lfwf_encoder.h>

#include <array>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>

namespace {

template <size_t D, typename Pixels>
void expectWaveletRoundTrip(
    const std::array<size_t, D> &image_size,
    const std::array<size_t, D> &block_size,
    uint8_t depth_bits,
    bool predicted,
    Pixels pixels) {
  std::stringstream stream;
  LFWFEncoder<D> encoder;
  encoder.create(stream, image_size, block_size, depth_bits, 0, predicted);
  encoder.encodeStream(pixels, stream);

  LFWFDecoder<D> decoder;
  decoder.open(stream);
  std::map<std::array<size_t, D>, std::array<uint16_t, 3>> decoded;
  decoder.decodeStream(stream, [&](const auto &position, const auto &rgb) {
    decoded[position] = rgb;
  });

  size_t expected_size = 1;
  for (const size_t extent : image_size) {
    expected_size *= extent;
  }
  ASSERT_EQ(decoded.size(), expected_size);
  for (const auto &[position, rgb] : decoded) {
    EXPECT_EQ(rgb, pixels(position));
  }
}

template <size_t D, typename Pixels>
std::string compressWavelet(
    const std::array<size_t, D> &image_size,
    const std::array<size_t, D> &block_size,
    uint8_t depth_bits,
    bool predicted,
    Pixels pixels) {
  std::stringstream stream;
  LFWFEncoder<D> encoder;
  encoder.create(stream, image_size, block_size, depth_bits, 0, predicted);
  encoder.encodeStream(pixels, stream);
  return stream.str();
}

}

TEST(LfwfRoundTrip, ReconstructsNonAlignedTwoDimensionalImage) {
  const auto pixels = [](const std::array<size_t, 2> &position) {
    const uint16_t value = (position[0] * 7 + position[1] * 13) % 256;
    return std::array<uint16_t, 3> {value, static_cast<uint16_t>(255 - value), static_cast<uint16_t>(position[0])};
  };
  expectWaveletRoundTrip<2>({13, 10}, {8, 8}, 8, false, pixels);
}

TEST(LfwfRoundTrip, ReconstructsOddThreeDimensionalImage) {
  const auto pixels = [](const std::array<size_t, 3> &position) {
    const uint16_t value = (position[0] * 11 + position[1] * 23 + position[2] * 41) % 256;
    return std::array<uint16_t, 3> {value, static_cast<uint16_t>(255 - value), static_cast<uint16_t>(position[2] * 31)};
  };
  expectWaveletRoundTrip<3>({5, 3, 3}, {4, 2, 2}, 8, false, pixels);
}

TEST(LfwfRoundTrip, ReconstructsFourDimensionalImage) {
  const auto pixels = [](const std::array<size_t, 4> &position) {
    const uint16_t value = (position[0] * 7 + position[1] * 13 + position[2] * 29 + position[3] * 53) % 256;
    return std::array<uint16_t, 3> {value, static_cast<uint16_t>(255 - value), static_cast<uint16_t>((position[2] + position[3]) * 17)};
  };
  expectWaveletRoundTrip<4>({3, 2, 2, 2}, {2, 2, 2, 2}, 8, false, pixels);
}

TEST(LfwfRoundTrip, ReconstructsPredictedImage) {
  const auto pixels = [](const std::array<size_t, 2> &position) {
    const uint16_t value = (position[0] * 17 + position[1] * 5) % 256;
    return std::array<uint16_t, 3> {value, value, value};
  };
  expectWaveletRoundTrip<2>({9, 7}, {4, 4}, 8, true, pixels);
}

TEST(LfwfRoundTrip, ReconstructsSixteenBitSamples) {
  const auto pixels = [](const std::array<size_t, 2> &position) {
    const uint16_t value = position[0] * 10000 + position[1] * 7000;
    return std::array<uint16_t, 3> {value, static_cast<uint16_t>(65535 - value), static_cast<uint16_t>(value / 2)};
  };
  expectWaveletRoundTrip<2>({3, 3}, {2, 2}, 16, false, pixels);
}

TEST(LfwfRoundTrip, CompressionIsDeterministic) {
  const auto pixels = [](const std::array<size_t, 2> &position) {
    const uint16_t value = (position[0] * 31 + position[1] * 19) % 256;
    return std::array<uint16_t, 3> {value, static_cast<uint16_t>(255 - value), value};
  };
  EXPECT_EQ(
      compressWavelet<2>({8, 6}, {4, 4}, 8, false, pixels),
      compressWavelet<2>({8, 6}, {4, 4}, 8, false, pixels));
}
