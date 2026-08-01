#include <gtest/gtest.h>

#include <components/bitstream.h>
#include <components/dct.h>
#include <dct_block_stream.h>
#include <dct_block_transformer.h>

#include "bitstream_io.h"

#include <array>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

TEST(Dct, OneDimensionalRoundTrip) {
  {
    const std::array<size_t, 1> size {8};
    const std::vector<float> expected {3.0F, -2.0F, 7.0F, 1.0F, 0.0F, 4.0F, -5.0F, 9.0F};
    std::vector<float> values = expected;
  DCTCoefs<1> coefficients(size);
    auto sample = [&](size_t index) -> float & { return values[index]; };

    fdct<1>(size, coefficients, sample);
    idct<1>(size, coefficients, sample);

    for (size_t i = 0; i < values.size(); ++i) {
      EXPECT_NEAR(values[i], expected[i], 0.0001F);
    }
  }
}

TEST(Dct, PreservesRoundingAtHalfBoundary) {
  {
    const std::array<size_t, 1> block_size {8};
    DynamicBlock<float, 1> block(block_size);
    const std::array<float, 8> input {-51, -122, 87, -32, 21, 64, -84, 27};
    for (size_t i = 0; i < input.size(); ++i) {
      block[i] = input[i];
    }

    DCTCoefs<1> coefficients(block_size);
    dctForward(block, coefficients, 0);

    EXPECT_EQ(block[7], 39);
  }
}

TEST(DctBlockStream, RejectsNonfiniteCoefficient) {
  DynamicBlock<float, 1> block({1});
  block[0] = std::numeric_limits<float>::infinity();
  std::stringstream stream;
  OBitstream output(byteSink(stream));
  CABACEncoder encoder([&](bool bit) { output.writeBit(bit); });
  DCTBlockStream<1> block_stream({1});

  EXPECT_THROW(encodeDctBlock(block_stream, block, encoder), std::overflow_error);
}
