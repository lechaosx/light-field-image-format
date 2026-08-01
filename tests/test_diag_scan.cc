#include <components/diag_scan.h>

#include <gtest/gtest.h>

#include <array>
#include <concepts>
#include <ranges>
#include <vector>

static_assert(std::same_as<
              std::ranges::range_reference_t<decltype(
                  diagonalScan(std::array<size_t, 2> {}, 0))>,
              const std::array<size_t, 2> &>);

TEST(DiagonalScan, PreservesOrderWithinEachDiagonal) {
  const std::array<size_t, 2> size {3, 2};
  std::vector<std::array<size_t, 2>> positions;

  for (size_t diagonal {}; diagonal < 4; ++diagonal) {
    for (const auto &position : diagonalScan(size, diagonal)) {
      positions.push_back(position);
    }
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

TEST(DiagonalScan, InvalidDiagonalProducesNoPositions) {
  const std::array<size_t, 3> size {2, 3, 2};
  size_t positions {};
  for ([[maybe_unused]] const auto &position :
       diagonalScan(size, 5)) {
    ++positions;
  }
  EXPECT_EQ(positions, 0);
}
