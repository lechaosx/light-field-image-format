#include <gtest/gtest.h>

#include <components/traversal_table.h>

#include <array>

TEST(TraversalTable, OrdersRadiusFromNearToFarWithStableTies) {
  TraversalTable<2> table({3, 3});
  constructByRadius(table);

  const std::array<size_t, 9> expected {0, 1, 4, 2, 3, 6, 5, 7, 8};
  for (size_t i = 0; i < expected.size(); i++) {
    EXPECT_EQ(table[i], expected[i]);
  }
}

TEST(TraversalTable, OrdersDiagonalsWithStableTies) {
  TraversalTable<2> table({3, 3});
  constructByDiagonals(table);

  const std::array<size_t, 9> expected {0, 1, 3, 2, 4, 6, 5, 7, 8};
  for (size_t i = 0; i < expected.size(); i++) {
    EXPECT_EQ(table[i], expected[i]);
  }
}
