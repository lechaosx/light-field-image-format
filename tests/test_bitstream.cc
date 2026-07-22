#include <gtest/gtest.h>

#include <components/bitstream.h>

#include <sstream>
#include <streambuf>
#include <vector>

namespace {

class FailingBuffer: public std::streambuf {
protected:
  int_type overflow(int_type) override {
    return traits_type::eof();
  }
};

} // namespace

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

TEST(Bitstream, DestructorDoesNotFlushPartialByte) {
  std::ostringstream stream;
  {
    OBitstream output(stream);
    output.writeBit(true);
  }
  EXPECT_TRUE(stream.str().empty());
}

TEST(Bitstream, ReadBitReportsEndOfStream) {
  std::istringstream stream(std::string(1, '\0'));
  IBitstream input(stream);
  for (size_t i = 0; i < 8; ++i) {
    EXPECT_FALSE(input.readBit());
  }
  EXPECT_THROW(input.readBit(), std::ios_base::failure);
}

TEST(Bitstream, FlushReportsOutputFailure) {
  FailingBuffer buffer;
  std::ostream stream(&buffer);
  OBitstream output(stream);
  output.writeBit(true);
  EXPECT_THROW(output.flush(), std::ios_base::failure);
}
