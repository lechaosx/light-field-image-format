#include <gtest/gtest.h>

#include <components/cabac.h>
#include <components/bitstream.h>

#include <sstream>
#include <vector>

namespace {

std::vector<bool> context_roundtrip(const std::vector<bool> &bits) {
  std::stringstream buffer;
  OBitstream obs {};
  {
    CABACEncoder encoder { [&](bool bit) { obs.writeBit(buffer, bit); } };
    CABAC::ContextModel context {};
    for (const bool bit : bits) {
      encoder.encodeBit(context, bit);
    }
    encoder.terminate();
  }
  obs.flush(buffer);

  IBitstream ibs {};
  CABACDecoder decoder { [&] { return ibs.readBit(buffer); } };
  CABAC::ContextModel context {};

  std::vector<bool> decoded;
  for (size_t i = 0; i < bits.size(); i++) {
    decoded.push_back(decoder.decodeBit(context));
  }
  return decoded;
}

std::vector<bool> bypass_roundtrip(const std::vector<bool> &bits) {
  std::stringstream buffer;
  OBitstream obs {};
  {
    CABACEncoder encoder { [&](bool bit) { obs.writeBit(buffer, bit); } };
    for (const bool bit : bits) {
      encoder.encodeBitBypass(bit);
    }
    encoder.terminate();
  }
  obs.flush(buffer);

  IBitstream ibs {};
  CABACDecoder decoder { [&] { return ibs.readBit(buffer); } };

  std::vector<bool> decoded;
  for (size_t i = 0; i < bits.size(); i++) {
    decoded.push_back(decoder.decodeBitBypass());
  }
  return decoded;
}

} // namespace

TEST(Cabac, RoundTripsContextCodedBits) {
  std::vector<bool> bits;
  for (int i = 0; i < 300; i++) {
    bits.push_back((i % 4) != 0); // skewed distribution exercises MPS/LPS adaptation
  }
  EXPECT_EQ(context_roundtrip(bits), bits);
}

TEST(Cabac, RoundTripsBypassCodedBits) {
  std::vector<bool> bits;
  for (int i = 0; i < 128; i++) {
    bits.push_back((i * 5 + 1) % 3 == 0);
  }
  EXPECT_EQ(bypass_roundtrip(bits), bits);
}
