#include <gtest/gtest.h>

#include <components/bitstream.h>
#include <components/endian.h>

#include "bitstream_io.h"

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

}

TEST(Bitstream, RoundTripsAcrossByteBoundaries) {
  std::vector<bool> expected;
  for (int i = 0; i < 257; ++i) {
    expected.push_back((i * 7 + 3) % 5 < 2);
  }

  std::stringstream stream;
  OBitstream output(byteSink(stream));
  output.write(expected);
  output.flush();

  IBitstream input(byteSource(stream));
  EXPECT_EQ(input.read(expected.size()), expected);
}

TEST(Bitstream, SupportsCallableByteSourceAndSink) {
  std::vector<uint8_t> bytes;
  OBitstream output([&](uint8_t byte) {
    bytes.push_back(byte);
  });
  output.write({true, false, true, true, false});
  output.flush();

  size_t next_byte {};
  IBitstream input([&] {
    return bytes.at(next_byte++);
  });
  EXPECT_EQ(input.read(5), (std::vector<bool> {true, false, true, true, false}));
}

TEST(Bitstream, FlushWritesPartialByteLeastSignificantBitFirst) {
  std::stringstream stream;
  OBitstream output(byteSink(stream));
  output.write({true, false, true, true, false});
  output.flush();

  ASSERT_EQ(stream.str().size(), 1U);
  EXPECT_EQ(static_cast<unsigned char>(stream.str()[0]), 0x0d);
}

TEST(Bitstream, DestructorDoesNotFlushPartialByte) {
  std::ostringstream stream;
  {
    OBitstream output(byteSink(stream));
    output.writeBit(true);
  }
  EXPECT_TRUE(stream.str().empty());
}

TEST(Bitstream, ReadBitReportsEndOfStream) {
  std::istringstream stream(std::string(1, '\0'));
  IBitstream input(byteSource(stream));
  for (size_t i = 0; i < 8; ++i) {
    EXPECT_FALSE(input.readBit());
  }
  EXPECT_THROW(input.readBit(), std::ios_base::failure);
}

TEST(Bitstream, FlushReportsOutputFailure) {
  FailingBuffer buffer;
  std::ostream stream(&buffer);
  OBitstream output(byteSink(stream));
  output.writeBit(true);
  EXPECT_THROW(output.flush(), std::ios_base::failure);
}

TEST(Bitstream, FixedWidthReadReportsTruncation) {
  std::istringstream stream(std::string(1, '\0'));
  EXPECT_THROW(readValueFromStream<uint16_t>(stream), std::ios_base::failure);
}

TEST(Bitstream, FixedWidthWriteReportsOutputFailure) {
  FailingBuffer buffer;
  std::ostream stream(&buffer);
  EXPECT_THROW(writeValueToStream<uint16_t>(42, stream), std::ios_base::failure);
}

TEST(Bitstream, FixedWidthValuesUseBigEndianByteOrder) {
  std::ostringstream output;
  writeValueToStream<uint32_t>(0x01020304, output);
  EXPECT_EQ(output.str(), std::string("\x01\x02\x03\x04", 4));

  std::istringstream input(std::string("\x05\x06\x07\x08", 4));
  EXPECT_EQ(readValueFromStream<uint32_t>(input), 0x05060708U);
}
