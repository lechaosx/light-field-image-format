#include <gtest/gtest.h>

#include <components/bitstream.h>
#include <components/cabac.h>

#include <sstream>
#include <vector>

namespace {

std::vector<bool> contextRoundTrip(const std::vector<bool> &bits) {
  std::stringstream stream;
  OBitstream output(stream);
  CABACEncoder encoder;
  encoder.init(output);
  CABAC::ContextModel encode_context {};

  for (const bool bit : bits) {
    encoder.encodeBit(encode_context, bit);
  }
  encoder.terminate();
  output.flush();

  IBitstream input(stream);
  CABACDecoder decoder;
  decoder.init(input);
  CABAC::ContextModel decode_context {};

  std::vector<bool> decoded;
  for (size_t i = 0; i < bits.size(); ++i) {
    decoded.push_back(decoder.decodeBit(decode_context));
  }
  decoder.terminate();
  return decoded;
}

std::vector<bool> bypassRoundTrip(const std::vector<bool> &bits) {
  std::stringstream stream;
  OBitstream output(stream);
  CABACEncoder encoder;
  encoder.init(output);

  for (const bool bit : bits) {
    encoder.encodeBitBypass(bit);
  }
  encoder.terminate();
  output.flush();

  IBitstream input(stream);
  CABACDecoder decoder;
  decoder.init(input);

  std::vector<bool> decoded;
  for (size_t i = 0; i < bits.size(); ++i) {
    decoded.push_back(decoder.decodeBitBypass());
  }
  decoder.terminate();
  return decoded;
}

}

TEST(Cabac, RoundTripsAdaptiveContextBits) {
  std::vector<bool> bits;
  for (int i = 0; i < 300; ++i) {
    bits.push_back((i % 4) != 0);
  }
  EXPECT_EQ(contextRoundTrip(bits), bits);
}

TEST(Cabac, RoundTripsBypassBits) {
  std::vector<bool> bits;
  for (int i = 0; i < 128; ++i) {
    bits.push_back((i * 5 + 1) % 3 == 0);
  }
  EXPECT_EQ(bypassRoundTrip(bits), bits);
}

TEST(Cabac, RoundTripsLargeUnaryValues) {
  const std::vector<uint64_t> expected {0, 1, 2, 255, 65535, 131071};
  std::stringstream stream;
  OBitstream output(stream);
  CABACEncoder encoder;
  encoder.init(output);
  CABAC::ContextModel encode_context {};
  for (const uint64_t value : expected) {
    encoder.encodeU(encode_context, value);
  }
  encoder.terminate();

  IBitstream input(stream);
  CABACDecoder decoder;
  decoder.init(input);
  CABAC::ContextModel decode_context {};
  std::vector<uint64_t> decoded;
  for (size_t i = 0; i < expected.size(); ++i) {
    decoded.push_back(decoder.decodeU(decode_context));
  }
  EXPECT_EQ(decoded, expected);
}
