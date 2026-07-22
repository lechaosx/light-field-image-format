#include <gtest/gtest.h>

#include <lfif/format.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::stringstream streamFor(const std::vector<uint8_t> &bytes) {
  return std::stringstream(std::string(bytes.begin(), bytes.end()));
}

lfif::Header twoDimensionalWaveletHeader() {
  return {
      .extents = {640, 480},
      .block_extents = {8, 8},
      .sample_depth = 8,
      .channels = 3,
      .sample_format = lfif::SampleFormat::unsigned_integer,
      .transform = lfif::Transform::wavelet,
      .entropy_codec = lfif::EntropyCodec::cabac,
      .color_space = lfif::ColorSpace::rgb,
      .discarded_bits = 0,
      .prediction = true,
      .disparity_compensated = false,
      .disparity_shift = {0, 0},
      .payload_size = 0x0102030405060708,
  };
}

}

TEST(Format, SerializesCanonicalHeaderBytes) {
  const std::vector<uint8_t> expected {
      0x4c, 0x46, 0x49, 0x46, 0x0d, 0x0a, 0x1a, 0x0a,
      0x01, 0x00, 0x00, 0x50,
      0x00, 0x00, 0x00, 0x01,
      0x02, 0x03, 0x08, 0x01, 0x01, 0x01, 0x01, 0x00,
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x80,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xe0,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
  };

  EXPECT_EQ(lfif::serializeHeader(twoDimensionalWaveletHeader()), expected);
}

TEST(Format, RoundTripsFourDimensionalDctMetadata) {
  const lfif::Header expected {
      .extents = {1920, 1080, 15, 15},
      .block_extents = {8, 8, 4, 4},
      .sample_depth = 10,
      .channels = 3,
      .sample_format = lfif::SampleFormat::unsigned_integer,
      .transform = lfif::Transform::dct,
      .entropy_codec = lfif::EntropyCodec::cabac,
      .color_space = lfif::ColorSpace::rgb,
      .discarded_bits = 4,
      .prediction = true,
      .disparity_compensated = true,
      .disparity_shift = {-2, 3},
      .payload_size = 123456,
  };

  auto input = streamFor(lfif::serializeHeader(expected));
  EXPECT_EQ(lfif::parseHeader(input), expected);
}

TEST(Format, RejectsMalformedAndUnsupportedHeaders) {
  const auto valid = lfif::serializeHeader(twoDimensionalWaveletHeader());

  auto bad_magic = valid;
  bad_magic[0] = 0;
  auto bad_magic_stream = streamFor(bad_magic);
  EXPECT_THROW(lfif::parseHeader(bad_magic_stream), std::runtime_error);

  auto bad_version = valid;
  bad_version[8] = 2;
  auto bad_version_stream = streamFor(bad_version);
  EXPECT_THROW(lfif::parseHeader(bad_version_stream), std::runtime_error);

  auto bad_flags = valid;
  bad_flags[15] = 0x80;
  auto bad_flags_stream = streamFor(bad_flags);
  EXPECT_THROW(lfif::parseHeader(bad_flags_stream), std::runtime_error);

  auto truncated = valid;
  truncated.resize(31);
  auto truncated_stream = streamFor(truncated);
  EXPECT_THROW(lfif::parseHeader(truncated_stream), std::runtime_error);

  for (const size_t identifier : {19U, 20U, 21U, 22U}) {
    auto unknown_identifier = valid;
    unknown_identifier[identifier] = 0xff;
    auto unknown_identifier_stream = streamFor(unknown_identifier);
    EXPECT_THROW(lfif::parseHeader(unknown_identifier_stream), std::runtime_error);
  }

  auto zero_extent = valid;
  std::fill(zero_extent.begin() + 48, zero_extent.begin() + 56, 0);
  auto zero_extent_stream = streamFor(zero_extent);
  EXPECT_THROW(lfif::parseHeader(zero_extent_stream), std::runtime_error);

  auto zero_block_extent = valid;
  std::fill(zero_block_extent.begin() + 64, zero_block_extent.begin() + 72, 0);
  auto zero_block_extent_stream = streamFor(zero_block_extent);
  EXPECT_THROW(lfif::parseHeader(zero_block_extent_stream), std::runtime_error);

  auto overflowing_extents = valid;
  std::fill(overflowing_extents.begin() + 48, overflowing_extents.begin() + 56, 0xff);
  auto overflowing_extents_stream = streamFor(overflowing_extents);
  EXPECT_THROW(lfif::parseHeader(overflowing_extents_stream), std::runtime_error);

  auto truncated_extension = valid;
  truncated_extension[9] = 1;
  truncated_extension[11] = 0x52;
  truncated_extension.push_back(0xaa);
  auto truncated_extension_stream = streamFor(truncated_extension);
  EXPECT_THROW(lfif::parseHeader(truncated_extension_stream), std::runtime_error);
}

TEST(Format, RejectsLegacyContainerHeadersExplicitly) {
  for (const std::string name : {
           "legacy-lfif-2d-header.lfif",
           "legacy-lfif-4d-header.lfif",
       }) {
    std::ifstream input(std::string(LFIF_TEST_FIXTURE_DIR) + "/" + name, std::ios::binary);
    ASSERT_TRUE(input);
    try {
      static_cast<void>(lfif::parseHeader(input));
      FAIL() << "legacy header was accepted: " << name;
    } catch (const std::runtime_error &error) {
      EXPECT_STREQ(error.what(), "unsupported legacy LFIF format");
    }
  }
}

TEST(Format, SkipsExtensionsFromNewerMinorVersions) {
  auto extended = lfif::serializeHeader(twoDimensionalWaveletHeader());
  extended[9] = 1;
  extended[11] = 0x52;
  extended.push_back(0xaa);
  extended.push_back(0x55);

  auto input = streamFor(extended);
  EXPECT_EQ(lfif::parseHeader(input), twoDimensionalWaveletHeader());
}

TEST(Format, RejectsInvalidMetadataBeforeSerialization) {
  auto header = twoDimensionalWaveletHeader();
  header.extents[0] = 0;
  EXPECT_THROW(lfif::serializeHeader(header), std::invalid_argument);

  header = twoDimensionalWaveletHeader();
  header.block_extents.pop_back();
  EXPECT_THROW(lfif::serializeHeader(header), std::invalid_argument);

  header = twoDimensionalWaveletHeader();
  header.transform = static_cast<lfif::Transform>(255);
  EXPECT_THROW(lfif::serializeHeader(header), std::invalid_argument);

  header = twoDimensionalWaveletHeader();
  header.discarded_bits = 9;
  EXPECT_THROW(lfif::serializeHeader(header), std::invalid_argument);
}
