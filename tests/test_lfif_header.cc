#include <gtest/gtest.h>

#include <components/lfif_header.h>

#include <cstdint>
#include <sstream>
#include <vector>

TEST(LfifHeader, WritesStableBigEndianBytes) {
  const LFIFHeader<2> header {
    {0x0102030405060708ULL, 0x1112131415161718ULL},
    {8, 16},
    8,
    3,
    true,
  };

  std::ostringstream stream;
  header.write(stream);
  const std::string bytes = stream.str();
  const std::vector<uint8_t> actual(bytes.begin(), bytes.end());
  const std::vector<uint8_t> expected {
    0x08, 0x03, 0x01,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
  };

  EXPECT_EQ(actual, expected);
}

TEST(LfifHeader, RoundTripsEveryField) {
  const LFIFHeader<3> expected {{13, 9, 5}, {8, 4, 2}, 10, 2, true};
  std::stringstream stream;
  expected.write(stream);

  LFIFHeader<3> actual {};
  actual.read(stream);

  EXPECT_EQ(actual.size, expected.size);
  EXPECT_EQ(actual.block_size, expected.block_size);
  EXPECT_EQ(actual.depth_bits, expected.depth_bits);
  EXPECT_EQ(actual.discarded_bits, expected.discarded_bits);
  EXPECT_EQ(actual.predicted, expected.predicted);
}
