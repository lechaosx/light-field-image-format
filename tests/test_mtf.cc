#include <gtest/gtest.h>

#include "mtf_reference.h"

#include <array>
#include <cstdint>
#include <map>
#include <string_view>
#include <vector>

namespace {

// Classic move-to-front: the found symbol is rotated to the front each step.
auto base = [](std::vector<uint64_t> &dictionary, size_t k) { updateBase(dictionary, k); };

template <typename Factory>
void expect_variant_roundtrip(std::string_view name, Factory make_update) {
  SCOPED_TRACE(name);

  std::vector<int64_t> original;
  for (int i = 0; i < 200; i++) {
    original.push_back((i * 37 + 11) % 17);
  }

  std::vector<int64_t> transformed = original;
  std::vector<uint64_t> encode_dictionary;
  auto encode_update = make_update();
  moveToFront(encode_dictionary, transformed, encode_update);

  std::vector<int64_t> restored = transformed;
  std::vector<uint64_t> decode_dictionary;
  auto decode_update = make_update();
  moveFromFront(decode_dictionary, restored, decode_update);

  EXPECT_EQ(restored, original);
}

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

TEST(MoveToFront, RoundTripsEveryPreservedUpdateVariant) {
  expect_variant_roundtrip("base", [] {
    return [](auto &dictionary, size_t i) { updateBase(dictionary, i); };
  });
  expect_variant_roundtrip("m1ff", [] {
    return [](auto &dictionary, size_t i) { updateM1ff(dictionary, i); };
  });
  expect_variant_roundtrip("m1ff2", [] {
    return [previous = std::array<uint64_t, 2> {}](auto &dictionary, size_t i) mutable {
      updateM1ff2(dictionary, i, previous.data());
    };
  });
  expect_variant_roundtrip("sticky", [] {
    return [](auto &dictionary, size_t i) { updateSticky(dictionary, i, 0.5); };
  });
  expect_variant_roundtrip("k-step", [] {
    return [](auto &dictionary, size_t i) { update_k(dictionary, i, 3); };
  });
  expect_variant_roundtrip("count-threshold", [] {
    return [counts = std::map<uint64_t, size_t> {}](auto &dictionary, size_t i) mutable {
      updateC(dictionary, i, 3, counts);
    };
  });
}
