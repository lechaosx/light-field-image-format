#include <gtest/gtest.h>

#include <components/bitstream.h>
#include <components/dwt.h>
#include <dwt_block_stream.h>
#include <dwt_block_transformer.h>

#include <array>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

TEST(Dwt, RoundTripsOddLengthSignal) {
  {
    const std::array<size_t, 1> size {7};
    const std::vector<int32_t> expected {5, -2, 9, 3, -7, 4, 11};
    std::vector<int32_t> values = expected;
    auto sample = [&](size_t index) -> int32_t & { return values[index]; };

    fdwt<1>(size, sample);
    idwt<1>(size, sample);

    EXPECT_EQ(values, expected);
  }
}

TEST(Dwt, RoundTripsTwoDimensionalSignal) {
  {
    const std::array<size_t, 2> size {3, 4};
    std::vector<int32_t> expected(12);
    for (size_t i = 0; i < expected.size(); ++i) {
      expected[i] = static_cast<int32_t>((i * 13) % 17) - 8;
    }
    std::vector<int32_t> values = expected;
    auto sample = [&](size_t index) -> int32_t & { return values[index]; };

    fdwt<2>(size, sample);
    idwt<2>(size, sample);

    EXPECT_EQ(values, expected);
  }
}

TEST(DwtBlockStream, RoundTripsLargeSignedCoefficients) {
  const std::array<size_t, 2> size {3, 3};
  DynamicBlock<int32_t, 2> expected(size);
  const std::array<int32_t, 9> coefficients {0, 1, -1, 2, -2, 65535, -65535, 131071, -131071};
  for (size_t i = 0; i < coefficients.size(); ++i) {
    expected[i] = coefficients[i];
  }

  std::stringstream stream;
  OBitstream output(stream);
  CABACEncoder encoder;
  encoder.init(output);
  DWTBlockStreamEncoder<2> block_encoder {};
  block_encoder.encodeBlock(expected, encoder);
  encoder.terminate();

  IBitstream input(stream);
  CABACDecoder decoder;
  decoder.init(input);
  DynamicBlock<int32_t, 2> decoded(size);
  DWTBlockStreamDecoder<2> block_decoder {};
  block_decoder.decodeBlock(decoder, decoded);

  for (size_t i = 0; i < coefficients.size(); ++i) {
    EXPECT_EQ(decoded[i], expected[i]);
  }
}

TEST(DwtBlockStream, RejectsUnrepresentableSignedMagnitude) {
  DynamicBlock<int32_t, 1> block({1});
  block[0] = std::numeric_limits<int32_t>::min();
  std::stringstream stream;
  OBitstream output(stream);
  CABACEncoder encoder;
  encoder.init(output);
  DWTBlockStreamEncoder<1> block_encoder;

  EXPECT_THROW(block_encoder.encodeBlock(block, encoder), std::overflow_error);
}

TEST(Dwt, RejectsInverseArithmeticOverflow) {
  const std::array<size_t, 1> size {2};
  std::array<int32_t, 2> values {
      std::numeric_limits<int32_t>::max(),
      std::numeric_limits<int32_t>::max(),
  };
  auto sample = [&](size_t index) -> int32_t & { return values[index]; };

  EXPECT_THROW(idwt<1>(size, sample), std::overflow_error);
}

TEST(DwtBlockTransformer, RejectsInverseBitRestorationOverflow) {
  DWTBlockTransformer<1> transformer(1);
  for (const int32_t coefficient : {
           std::numeric_limits<int32_t>::max(),
           std::numeric_limits<int32_t>::min(),
       }) {
    DynamicBlock<int32_t, 1> block({1});
    block[0] = coefficient;
    EXPECT_THROW(transformer.inversePass(block), std::overflow_error);
  }
}
