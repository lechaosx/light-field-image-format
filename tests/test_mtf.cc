#include <gtest/gtest.h>

#include "mtf_reference.h"

#include <cstdint>
#include <vector>

namespace {

// Classic move-to-front: the found symbol is rotated to the front each step.
auto base = [](std::vector<uint64_t> &dictionary, size_t k) { updateBase(dictionary, k); };

} // namespace

TEST(MoveToFront, EncodesAKnownVector) {
  std::vector<uint64_t> dictionary {0, 1, 2, 3};
  std::vector<int64_t> data {2, 2, 1, 0};

  moveToFront(dictionary, data, base);

  EXPECT_EQ(data, (std::vector<int64_t>{2, 0, 2, 2}));
}

TEST(MoveToFront, RoundTripsThroughAGrowingDictionary) {
  std::vector<int64_t> original;
  for (int i = 0; i < 200; i++) {
    original.push_back((i * 37 + 11) % 17);
  }

  std::vector<int64_t> transformed = original;
  std::vector<uint64_t> encode_dictionary;
  moveToFront(encode_dictionary, transformed, base);

  std::vector<int64_t> restored = transformed;
  std::vector<uint64_t> decode_dictionary;
  moveFromFront(decode_dictionary, restored, base);

  EXPECT_EQ(restored, original);
  EXPECT_NE(transformed, original);
}
