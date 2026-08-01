#include "components/meta.h"

#include <gtest/gtest.h>

#include <array>
#include <vector>

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
