#include "components/meta.h"

#include <gtest/gtest.h>

#include <array>
#include <vector>

template<typename T>
concept TwoDimensionalStrideInput = requires(T size) {
  get_stride<2>(size);
};

static_assert(TwoDimensionalStrideInput<std::array<size_t, 2>>);
static_assert(!TwoDimensionalStrideInput<const size_t *>);

TEST(Meta, IteratesDimensionsInStorageOrder) {
  std::vector<std::array<size_t, 2>> positions;

  for (const auto &position :
       iterate_dimensions<2>(std::array<size_t, 2> {2, 3})) {
    positions.push_back(position);
  }

  EXPECT_EQ(positions, (std::vector<std::array<size_t, 2>> {
                           {0, 0},
                           {1, 0},
                           {0, 1},
                           {1, 1},
                           {0, 2},
                           {1, 2},
                       }));
}

TEST(Meta, IteratesBlocksInStorageOrder) {
  std::vector<std::array<size_t, 2>> positions;

  for (const auto &position : block_for<2>(
      std::array<size_t, 2> {1, 2},
      std::array<size_t, 2> {2, 3},
      std::array<size_t, 2> {6, 9})) {
    positions.push_back(position);
  }

  EXPECT_EQ(positions, (std::vector<std::array<size_t, 2>> {
                           {1, 2},
                           {3, 2},
                           {5, 2},
                           {1, 5},
                           {3, 5},
                           {5, 5},
                           {1, 8},
                           {3, 8},
                           {5, 8},
                       }));
}

TEST(Meta, IteratesCompileTimeCubeInStorageOrder) {
  std::vector<std::array<size_t, 2>> positions;

  for (const auto &position :
       iterate_dimensions<2>(std::array<size_t, 2> {2, 2})) {
    positions.push_back(position);
  }

  EXPECT_EQ(positions, (std::vector<std::array<size_t, 2>> {
                           {0, 0},
                           {1, 0},
                           {0, 1},
                           {1, 1},
                       }));
}

TEST(Meta, EmptyDimensionProducesNoPositions) {
  size_t positions {};
  for ([[maybe_unused]] const auto &position :
       iterate_dimensions<3>(std::array<size_t, 3> {2, 0, 3})) {
    ++positions;
  }
  EXPECT_EQ(positions, 0);
}

TEST(Meta, EmptyBlockRangeProducesNoPositions) {
  size_t positions {};
  for ([[maybe_unused]] const auto &position : block_for<2>(
      std::array<size_t, 2> {2, 0},
      std::array<size_t, 2> {1, 1},
      std::array<size_t, 2> {2, 3})) {
    ++positions;
  }
  EXPECT_EQ(positions, 0);
}
