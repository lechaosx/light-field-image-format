#include <gtest/gtest.h>

#include <dct_block_transformer.h>

#include <array>

TEST(Dct, PreservesBaselineRoundingAtHalfBoundary) {
  const std::array<size_t, 1> block_size {8};
  DynamicBlock<float, 1> block(block_size);

  const std::array<float, 8> input {-51, -122, 87, -32, 21, 64, -84, 27};
  for (size_t i = 0; i < input.size(); i++) {
    block[i] = input[i];
  }

  const DCTCoefs<1> coefs(block_size);
  dct_forward(block, coefs, 0);

  EXPECT_EQ(block[7], 39);
}
