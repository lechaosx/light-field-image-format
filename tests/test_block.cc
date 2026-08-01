#include <gtest/gtest.h>

#include <components/block.h>

#include <array>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

static_assert(!std::is_constructible_v<DynamicBlock<int, 2>, const size_t *>);

TEST(DynamicBlock, ValueInitializesReusedStorage) {
  {
    DynamicBlock<int, 1> dirty({4});
    dirty.fill(7);
  }
  {
    DynamicBlock<int, 1> clean({4});
    for (size_t i = 0; i < 4; ++i) {
      EXPECT_EQ(clean[i], 0);
    }
  }
}

TEST(DynamicBlock, CopiesOwnIndependentStorage) {
  {
    DynamicBlock<int, 1> original({2});
    original[0] = 1;
    original[1] = 2;

    DynamicBlock<int, 1> copy = original;
    copy[0] = 9;

    EXPECT_EQ(original[0], 1);
    EXPECT_EQ(copy[0], 9);
  }
}

TEST(DynamicBlock, SupportsStandardRaiiAllocation) {
  const auto block = std::make_unique<DynamicBlock<int, 1>>(std::array<size_t, 1> {2});
  EXPECT_EQ(block->size(), (std::array<size_t, 1> {2}));
}

TEST(DynamicBlock, RejectsDimensionProductOverflow) {
  EXPECT_THROW(
      (DynamicBlock<int, 2> {{std::numeric_limits<size_t>::max(), 2}}),
      std::length_error);
}
