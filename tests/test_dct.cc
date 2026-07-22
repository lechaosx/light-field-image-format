#include <gtest/gtest.h>

#include <components/dct.h>
#include <components/stack_allocator.h>
#include <dct_block_transformer.h>

#include <array>
#include <cmath>
#include <vector>

TEST(Dct, OneDimensionalRoundTrip) {
  StackAllocator::init(1024 * 1024);
  {
    const std::array<size_t, 1> size {8};
    const std::vector<float> expected {3.0F, -2.0F, 7.0F, 1.0F, 0.0F, 4.0F, -5.0F, 9.0F};
    std::vector<float> values = expected;
    DCTCoefs<1> coefficients(size.data());
    auto sample = [&](size_t index) -> float & { return values[index]; };

    fdct<1>(size, coefficients, sample);
    idct<1>(size, coefficients, sample);

    for (size_t i = 0; i < values.size(); ++i) {
      EXPECT_NEAR(values[i], expected[i], 0.0001F);
    }
  }
  StackAllocator::cleanup();
}

TEST(Dct, PreservesRoundingAtHalfBoundary) {
  StackAllocator::init(1024 * 1024);
  {
    const std::array<size_t, 1> block_size {8};
    DynamicBlock<float, 1> block(block_size);
    const std::array<float, 8> input {-51, -122, 87, -32, 21, 64, -84, 27};
    for (size_t i = 0; i < input.size(); ++i) {
      block[i] = input[i];
    }

    DCTBlockTransformer<1> transformer(block_size, 0);
    transformer.forwardPass(block);

    EXPECT_EQ(block[7], 39);
  }
  StackAllocator::cleanup();
}
