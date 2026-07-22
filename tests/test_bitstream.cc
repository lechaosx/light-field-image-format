#include <gtest/gtest.h>

#include <components/bitstream.h>

#include <sstream>
#include <vector>

TEST(Bitstream, RoundTripsAcrossByteBoundaries) {
  std::vector<bool> expected;
  for (int i = 0; i < 257; ++i) {
    expected.push_back((i * 7 + 3) % 5 < 2);
  }

  std::stringstream stream;
  OBitstream output(stream);
  output.write(expected);
  output.flush();

  IBitstream input(stream);
  EXPECT_EQ(input.read(expected.size()), expected);
}

TEST(Bitstream, FlushWritesPartialByteLeastSignificantBitFirst) {
  std::stringstream stream;
  OBitstream output(stream);
  output.write({true, false, true, true, false});
  output.flush();

  ASSERT_EQ(stream.str().size(), 1U);
  EXPECT_EQ(static_cast<unsigned char>(stream.str()[0]), 0x0d);
}
