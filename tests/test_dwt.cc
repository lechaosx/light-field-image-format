#include <gtest/gtest.h>

#include <components/dwt.h>
#include <components/stack_allocator.h>

#include <array>
#include <cstdint>
#include <vector>

TEST(Dwt, RoundTripsOddLengthSignal) {
  StackAllocator::init(1024 * 1024);
  {
    const std::array<size_t, 1> size {7};
    const std::vector<int32_t> expected {5, -2, 9, 3, -7, 4, 11};
    std::vector<int32_t> values = expected;
    auto sample = [&](size_t index) -> int32_t & { return values[index]; };

    fdwt<1>(size, sample);
    idwt<1>(size, sample);

    EXPECT_EQ(values, expected);
  }
  StackAllocator::cleanup();
}

TEST(Dwt, RoundTripsTwoDimensionalSignal) {
  StackAllocator::init(1024 * 1024);
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
  StackAllocator::cleanup();
}
