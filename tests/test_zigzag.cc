#include <gtest/gtest.h>

#include <components/zigzag.h>
#include "zigzag_oracle.h"

#include <array>
#include <vector>
#include <set>
#include <algorithm>

namespace {

// The generalized N-dimensional scan, as a flat sequence of visited positions.
template <size_t D>
std::vector<std::array<size_t, D>> zigzagOrder(std::array<size_t, D> size) {
  std::vector<std::array<size_t, D>> order;
  for (const auto &pos : zigzagScan<D>(size)) {
    order.push_back(pos);
  }
  return order;
}

template <size_t D>
std::vector<std::array<size_t, D>> zigzagOrderSkipFirst(std::array<size_t, D> size) {
  std::vector<std::array<size_t, D>> order;
  for (const auto &pos : zigzagScanSkipFirst<D>(size)) {
    order.push_back(pos);
  }
  return order;
}

// The same sequence as produced by the hardcoded per-dimension oracle.
template <size_t D>
std::vector<std::array<size_t, D>> oracleOrder(std::array<size_t, D> size) {
  std::vector<std::array<size_t, D>> order;
  auto record = [&](const size_t *pos) {
    std::array<size_t, D> a {};
    std::copy(pos, pos + D, a.begin());
    order.push_back(a);
  };
  if constexpr (D == 2) { oracle::zigzagScan2D(size.data(), record); }
  else if constexpr (D == 3) { oracle::zigzagScan3D(size.data(), record); }
  else if constexpr (D == 4) { oracle::zigzagScan4D(size.data(), record); }
  return order;
}

} // namespace

TEST(Zigzag, GeneralizedMatchesHardcoded2D) {
  EXPECT_EQ(zigzagOrder<2>({8, 8}), oracleOrder<2>({8, 8}));
  EXPECT_EQ(zigzagOrder<2>({4, 6}), oracleOrder<2>({4, 6}));
  EXPECT_EQ(zigzagOrder<2>({6, 4}), oracleOrder<2>({6, 4}));
  EXPECT_EQ(zigzagOrder<2>({2, 2}), oracleOrder<2>({2, 2}));
  EXPECT_EQ(zigzagOrder<2>({3, 7}), oracleOrder<2>({3, 7}));
}

TEST(Zigzag, GeneralizedMatchesHardcoded3D) {
  EXPECT_EQ(zigzagOrder<3>({4, 4, 4}), oracleOrder<3>({4, 4, 4}));
  EXPECT_EQ(zigzagOrder<3>({2, 3, 4}), oracleOrder<3>({2, 3, 4}));
  EXPECT_EQ(zigzagOrder<3>({3, 2, 5}), oracleOrder<3>({3, 2, 5}));
  EXPECT_EQ(zigzagOrder<3>({2, 2, 2}), oracleOrder<3>({2, 2, 2}));
}

TEST(Zigzag, GeneralizedMatchesHardcoded4D) {
  EXPECT_EQ(zigzagOrder<4>({2, 2, 2, 2}), oracleOrder<4>({2, 2, 2, 2}));
  EXPECT_EQ(zigzagOrder<4>({3, 2, 4, 2}), oracleOrder<4>({3, 2, 4, 2}));
  EXPECT_EQ(zigzagOrder<4>({2, 3, 2, 4}), oracleOrder<4>({2, 3, 2, 4}));
}

TEST(Zigzag, VisitsEveryCellExactlyOnce) {
  const auto order = zigzagOrder<3>({4, 4, 4});
  const std::set<std::array<size_t, 3>> distinct(order.begin(), order.end());

  EXPECT_EQ(order.size(), 64u);
  EXPECT_EQ(distinct.size(), 64u);
}

TEST(Zigzag, SkipFirstVisitsEveryCellExceptTheDcCoefficient) {
  const auto full = zigzagOrder<2>({8, 8});
  const auto skipped = zigzagOrderSkipFirst<2>({8, 8});

  std::set<std::array<size_t, 2>> expected(full.begin(), full.end());
  expected.erase({0, 0});
  const std::set<std::array<size_t, 2>> visited(skipped.begin(), skipped.end());

  EXPECT_EQ(skipped.size(), full.size() - 1);
  EXPECT_EQ(visited, expected);
}
