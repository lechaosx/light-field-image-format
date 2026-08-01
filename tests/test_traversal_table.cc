#include <gtest/gtest.h>

#include <components/traversal_table.h>

#include <array>
#include <sstream>

TEST(TraversalTable, OrdersRadiusFromNearToFarWithStableTies) {
  {
    TraversalTable<2> table({3, 3});
    constructByRadius(table);

    const std::array<size_t, 9> expected {0, 1, 4, 2, 3, 6, 5, 7, 8};
    for (size_t i = 0; i < expected.size(); ++i) {
      EXPECT_EQ(table[i], expected[i]);
    }
  }
}

TEST(TraversalTable, OrdersDiagonalsWithStableTies) {
  {
    TraversalTable<2> table({3, 3});
    constructByDiagonals(table);

    const std::array<size_t, 9> expected {0, 1, 3, 2, 4, 6, 5, 7, 8};
    for (size_t i = 0; i < expected.size(); ++i) {
      EXPECT_EQ(table[i], expected[i]);
    }
  }
}

TEST(TraversalTable, SerializesAcrossStorageWidthBoundary) {
  for (const auto &[extent, bytes_per_value] :
       std::array<std::array<size_t, 2>, 2> {{{256, 1}, {257, 2}}}) {
    TraversalTable<1> table({extent});
    for (size_t i = 0; i < extent; ++i) {
      table[i] = i;
    }

    std::stringstream stream;
    writeTraversalToStream(table, stream);
    EXPECT_EQ(stream.str().size(), extent * bytes_per_value);

    const TraversalTable<1> decoded =
        readTraversalFromStream<1>({extent}, stream);
    for (size_t i = 0; i < extent; ++i) {
      EXPECT_EQ(decoded[i], i);
    }
  }
}
