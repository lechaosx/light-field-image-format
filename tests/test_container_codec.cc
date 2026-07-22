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

void expectApproximateRoundTrip(
    const lfif::Header &metadata,
    const std::vector<lfif::Pixel> &expected,
    uint16_t tolerance) {
  std::stringstream stream;
  const lfif::Header written = lfif::writeImage(stream, metadata, expected);
  const lfif::DecodedImage decoded = lfif::readImage(stream);

  EXPECT_EQ(decoded.header, written);
  ASSERT_EQ(decoded.pixels.size(), expected.size());
  for (size_t pixel = 0; pixel < expected.size(); ++pixel) {
    for (size_t channel = 0; channel < expected[pixel].size(); ++channel) {
      EXPECT_NEAR(decoded.pixels[pixel][channel], expected[pixel][channel], tolerance);
    }
  }
}

}

TEST(ContainerCodec, RoundTripsWaveletImagesAcrossSupportedDimensions) {
  expectRoundTrip(header({7, 5}, {4, 4}), pixels(35));
  expectRoundTrip(header({3, 3, 2}, {2, 2, 2}), pixels(18));
  expectRoundTrip(header({3, 2, 2, 2}, {2, 2, 2, 2}), pixels(24));
}

TEST(ContainerCodec, RoundTripsPredictedWaveletImagesAcrossSupportedDimensions) {
  auto two_dimensions = header({2, 2}, {2, 2});
  two_dimensions.prediction = true;
  expectRoundTrip(two_dimensions, pixels(4));

  auto three_dimensions = header({2, 2, 2}, {2, 2, 2});
  three_dimensions.prediction = true;
  expectRoundTrip(three_dimensions, pixels(8));

  auto four_dimensions = header({2, 2, 2, 2}, {2, 2, 2, 2});
  four_dimensions.prediction = true;
  expectRoundTrip(four_dimensions, pixels(16));
}

TEST(ContainerCodec, RoundTripsPredictedSixteenBitWaveletSamples) {
  auto metadata = header({3, 3}, {2, 2});
  metadata.sample_depth = 16;
  metadata.prediction = true;
  std::vector<lfif::Pixel> samples(9);
  for (size_t i = 0; i < samples.size(); ++i) {
    samples[i] = {
        static_cast<uint16_t>(i * 7000),
        static_cast<uint16_t>(65535 - i * 7000),
        static_cast<uint16_t>(i * 3000),
    };
  }
  expectRoundTrip(metadata, samples);
}

TEST(ContainerCodec, PreservesLossyWaveletMetadataAndSampleBounds) {
  auto metadata = header({7, 5}, {4, 4});
  metadata.discarded_bits = 2;
  const std::vector<lfif::Pixel> original = pixels(35);

  std::stringstream stream;
  const lfif::Header written = lfif::writeImage(stream, metadata, original);
  const lfif::DecodedImage decoded = lfif::readImage(stream);

  EXPECT_EQ(decoded.header, written);
  ASSERT_EQ(decoded.pixels.size(), original.size());
  EXPECT_NE(decoded.pixels, original);
  for (const lfif::Pixel &pixel : decoded.pixels) {
    for (const uint16_t sample : pixel) {
      EXPECT_LE(sample, 255);
    }
  }
}

TEST(ContainerCodec, WritesDeterministicCompleteWaveletContainers) {
  const auto metadata = header({7, 5}, {4, 4});
  const auto samples = pixels(35);
  const auto encode = [&] {
    std::stringstream stream;
    lfif::writeImage(stream, metadata, samples);
    return stream.str();
  };

  EXPECT_EQ(encode(), encode());
}

TEST(ContainerCodec, RoundTripsExplicitDctVariant) {
  const std::vector<lfif::Pixel> constant(16, {80, 120, 160});
  expectRoundTrip(header({2, 2, 2, 2}, {2, 2, 2, 2}, lfif::Transform::dct), constant);
}

TEST(ContainerCodec, ReconstructsNonconstantDctSamplesWithinOneLevel) {
  expectApproximateRoundTrip(header({4, 4}, {4, 4}, lfif::Transform::dct), pixels(16), 1);
}

TEST(ContainerCodec, ReconstructsNonalignedThreeDimensionalDctSamplesWithinOneLevel) {
  expectApproximateRoundTrip(
      header({3, 2, 2}, {2, 2, 2}, lfif::Transform::dct), pixels(12), 1);
}

TEST(ContainerCodec, ReconstructsPredictedNonalignedFourDimensionalDctSamplesWithinTwoLevels) {
  auto metadata = header({3, 2, 2, 2}, {2, 2, 2, 2}, lfif::Transform::dct);
  metadata.prediction = true;
  expectApproximateRoundTrip(metadata, pixels(24), 2);
}

TEST(ContainerCodec, ReconstructsSixteenBitDctSamplesWithinOneLevel) {
  auto metadata = header({2, 2}, {2, 2}, lfif::Transform::dct);
  metadata.sample_depth = 16;
  const std::vector<lfif::Pixel> original {
      {0, 65535, 12345},
      {65535, 0, 54321},
      {10000, 30000, 50000},
      {50000, 30000, 10000},
  };
  expectApproximateRoundTrip(metadata, original, 1);
}

TEST(ContainerCodec, WritesDeterministicCompleteDctContainers) {
  auto metadata = header({3, 2, 2}, {2, 2, 2}, lfif::Transform::dct);
  metadata.prediction = true;
  const auto samples = pixels(12);
  const auto encode = [&] {
    std::stringstream stream;
    lfif::writeImage(stream, metadata, samples);
    return stream.str();
  };

  EXPECT_EQ(encode(), encode());
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

TEST(ContainerCodec, RejectsSamplesOutsideDeclaredDepthBeforeWriting) {
  std::stringstream stream;
  EXPECT_THROW(
      lfif::writeImage(stream, header({1, 1}, {1, 1}), std::vector<lfif::Pixel> {{{256, 0, 0}}}),
      std::invalid_argument);
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

TEST(ContainerCodec, RejectsMissingLargePayloadBeforeAllocatingItsDeclaredSize) {
  auto metadata = header({1, 1}, {1, 1});
  metadata.payload_size = uint64_t {1} << 32;
  const std::vector<uint8_t> encoded = lfif::serializeHeader(metadata);
  std::stringstream input(std::string(encoded.begin(), encoded.end()));

  try {
    static_cast<void>(lfif::readImage(input));
    FAIL() << "missing payload was accepted";
  } catch (const std::runtime_error &error) {
    EXPECT_STREQ(error.what(), "truncated LFIF payload");
  }
}
