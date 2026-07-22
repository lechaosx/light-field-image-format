#include <gtest/gtest.h>

#include <components/bitstream.h>

#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<bool> roundtrip(const std::vector<bool> &bits) {
  std::stringstream buffer;

  OBitstream out {};
  for (const bool bit : bits) {
    out.writeBit(buffer, bit);
  }
  out.flush(buffer);

  IBitstream in {};
  std::vector<bool> read;
  for (size_t i = 0; i < bits.size(); i++) {
    read.push_back(in.readBit(buffer));
  }
  return read;
}

} // namespace

TEST(Bitstream, RoundTripsSubBytePattern) {
  const std::vector<bool> bits {true, false, true, true, false};
  EXPECT_EQ(roundtrip(bits), bits);
}

TEST(Bitstream, RoundTripsAcrossManyByteBoundaries) {
  std::vector<bool> bits;
  for (int i = 0; i < 257; i++) {
    bits.push_back((i * 7 + 3) % 5 < 2);
  }
  EXPECT_EQ(roundtrip(bits), bits);
}

TEST(Bitstream, PacksBitsLeastSignificantFirst) {
  std::ostringstream buffer;
  OBitstream out {};
  for (const bool bit : {true, false, true, true, false}) {
    out.writeBit(buffer, bit);
  }

  EXPECT_TRUE(buffer.str().empty());
  out.flush(buffer);
  EXPECT_EQ(buffer.str(), std::string(1, static_cast<char>(0x0d)));
}

TEST(Bitstream, RequiresExplicitFlushForPartialByte) {
  std::ostringstream buffer;
  {
    OBitstream out {};
    out.writeBit(buffer, true);
  }

  EXPECT_TRUE(buffer.str().empty());
}
