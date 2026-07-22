#include <gtest/gtest.h>

#include <components/zigzag.h>

#include <algorithm>
#include <array>
#include <set>
#include <vector>

namespace {

template <size_t D>
std::vector<std::array<size_t, D>> zigzagOrder(const std::array<size_t, D> &size) {
  std::vector<std::array<size_t, D>> order;
  zigzagScan<D>(size.data(), [&](const size_t position[D]) {
    std::array<size_t, D> value {};
    std::copy(position, position + D, value.begin());
    order.push_back(value);
  });
  return order;
}

template <size_t D>
std::vector<std::array<size_t, D>> legacyZigzagOrder(const std::array<size_t, D> &size) {
  std::vector<std::array<size_t, D>> order;
  auto record = [&](const size_t *position) {
    std::array<size_t, D> value {};
    std::copy(position, position + D, value.begin());
    order.push_back(value);
  };
  if constexpr (D == 2) {
    zigzagScan2D(size.data(), record);
  } else if constexpr (D == 3) {
    zigzagScan3D(size.data(), record);
  } else if constexpr (D == 4) {
    zigzagScan4D(size.data(), record);
  }
  return order;
}

} // namespace

TEST(Zigzag, ScansKnownThreeByThreeOrder) {
  const std::vector<std::array<size_t, 2>> expected {
    {0, 0}, {0, 1}, {1, 0}, {2, 0}, {1, 1}, {0, 2}, {1, 2}, {2, 1}, {2, 2}
  };
  EXPECT_EQ(zigzagOrder<2>({3, 3}), expected);
}

TEST(Zigzag, GeneralizedMatchesLegacyTwoDimensionalScans) {
  EXPECT_EQ(zigzagOrder<2>({8, 8}), legacyZigzagOrder<2>({8, 8}));
  EXPECT_EQ(zigzagOrder<2>({4, 6}), legacyZigzagOrder<2>({4, 6}));
  EXPECT_EQ(zigzagOrder<2>({6, 4}), legacyZigzagOrder<2>({6, 4}));
  EXPECT_EQ(zigzagOrder<2>({3, 7}), legacyZigzagOrder<2>({3, 7}));
}

TEST(Zigzag, GeneralizedMatchesLegacyThreeDimensionalScans) {
  EXPECT_EQ(zigzagOrder<3>({4, 4, 4}), legacyZigzagOrder<3>({4, 4, 4}));
  EXPECT_EQ(zigzagOrder<3>({2, 3, 4}), legacyZigzagOrder<3>({2, 3, 4}));
  EXPECT_EQ(zigzagOrder<3>({3, 2, 5}), legacyZigzagOrder<3>({3, 2, 5}));
}

TEST(Zigzag, GeneralizedMatchesLegacyFourDimensionalScans) {
  EXPECT_EQ(zigzagOrder<4>({2, 2, 2, 2}), legacyZigzagOrder<4>({2, 2, 2, 2}));
  EXPECT_EQ(zigzagOrder<4>({3, 2, 4, 2}), legacyZigzagOrder<4>({3, 2, 4, 2}));
  EXPECT_EQ(zigzagOrder<4>({2, 3, 2, 4}), legacyZigzagOrder<4>({2, 3, 2, 4}));
}

TEST(Zigzag, VisitsEveryThreeDimensionalPositionOnce) {
  const auto order = zigzagOrder<3>({2, 3, 4});
  const std::set<std::array<size_t, 3>> positions(order.begin(), order.end());
  EXPECT_EQ(order.size(), 24U);
  EXPECT_EQ(positions.size(), 24U);
}
