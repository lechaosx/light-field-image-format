#include <gtest/gtest.h>

#include <lfif/codec.h>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

lfif::Header header(
    std::vector<uint64_t> extents,
    std::vector<uint64_t> block_extents,
    lfif::Transform transform = lfif::Transform::wavelet) {
  return {
      .extents = std::move(extents),
      .block_extents = std::move(block_extents),
      .sample_depth = 8,
      .channels = 3,
      .sample_format = lfif::SampleFormat::unsigned_integer,
      .transform = transform,
      .entropy_codec = lfif::EntropyCodec::cabac,
      .color_space = lfif::ColorSpace::rgb,
  };
}

std::vector<lfif::Pixel> pixels(size_t count) {
  std::vector<lfif::Pixel> result(count);
  for (size_t i = 0; i < count; ++i) {
    result[i] = {
        static_cast<uint16_t>((i * 17) % 256),
        static_cast<uint16_t>((i * 31 + 7) % 256),
        static_cast<uint16_t>((i * 47 + 3) % 256),
    };
  }
  return result;
}

void expectRoundTrip(const lfif::Header &metadata, const std::vector<lfif::Pixel> &expected) {
  std::stringstream stream;
  const lfif::Header written = lfif::writeImage(stream, metadata, expected);
  EXPECT_EQ(written.transform, metadata.transform);
  EXPECT_GT(written.payload_size, 0U);

  const std::string encoded = stream.str();
  std::stringstream header_stream(encoded);
  const lfif::Header parsed = lfif::parseHeader(header_stream);
  ASSERT_GE(header_stream.tellg(), 0);
  EXPECT_EQ(encoded.size() - static_cast<size_t>(header_stream.tellg()), parsed.payload_size);

  std::stringstream input(encoded);
  const lfif::DecodedImage decoded = lfif::readImage(input);
  EXPECT_EQ(decoded.header, written);
  EXPECT_EQ(decoded.pixels, expected);
}

} // namespace

TEST(ContainerCodec, RoundTripsWaveletImagesAcrossSupportedDimensions) {
  expectRoundTrip(header({7, 5}, {4, 4}), pixels(35));
  expectRoundTrip(header({3, 3, 2}, {2, 2, 2}), pixels(18));
  expectRoundTrip(header({3, 2, 2, 2}, {2, 2, 2, 2}), pixels(24));
}

TEST(ContainerCodec, RoundTripsExplicitDctVariant) {
  const std::vector<lfif::Pixel> constant(16, {80, 120, 160});
  expectRoundTrip(header({2, 2, 2, 2}, {2, 2, 2, 2}, lfif::Transform::dct), constant);
}

TEST(ContainerCodec, DoesNotWriteProgressToStandardError) {
  std::ostringstream diagnostics;
  std::streambuf *original = std::cerr.rdbuf(diagnostics.rdbuf());

  expectRoundTrip(header({2, 2}, {1, 1}), pixels(4));
  expectRoundTrip(
      header({2, 2, 2, 2}, {2, 2, 2, 2}, lfif::Transform::dct),
      std::vector<lfif::Pixel>(16, {80, 120, 160}));

  std::cerr.rdbuf(original);
  EXPECT_TRUE(diagnostics.str().empty()) << diagnostics.str();
}

TEST(ContainerCodec, RejectsPixelCountMismatchBeforeWriting) {
  std::stringstream stream;
  EXPECT_THROW(lfif::writeImage(stream, header({4, 4}, {2, 2}), pixels(15)), std::invalid_argument);
  EXPECT_TRUE(stream.str().empty());
}

TEST(ContainerCodec, RejectsUnsupportedCodecDimensionsBeforeWriting) {
  std::stringstream stream;
  EXPECT_THROW(lfif::writeImage(stream, header({8}, {4}), pixels(8)), std::invalid_argument);
  EXPECT_TRUE(stream.str().empty());
}

TEST(ContainerCodec, RejectsTruncatedPayload) {
  std::stringstream stream;
  lfif::writeImage(stream, header({4, 4}, {2, 2}), pixels(16));
  std::string encoded = stream.str();
  encoded.pop_back();

  std::stringstream input(encoded);
  EXPECT_THROW(lfif::readImage(input), std::runtime_error);
}
