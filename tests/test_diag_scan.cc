#include <components/diag_scan.h>

#include <gtest/gtest.h>

#include <array>
#include <vector>

TEST(DiagonalScan, PreservesOrderWithinEachDiagonal) {
  const std::array<size_t, 2> size {3, 2};
  std::vector<std::array<size_t, 2>> positions;

  for (size_t diagonal {}; diagonal < 4; ++diagonal) {
    diagonalScan(size, diagonal, [&](const auto &position) {
      positions.push_back(position);
    });
  }

  EXPECT_EQ(positions, (std::vector<std::array<size_t, 2>> {
                           {0, 0},
                           {1, 0},
                           {0, 1},
                           {2, 0},
                           {1, 1},
                           {2, 1},
                       }));
}
