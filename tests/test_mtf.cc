#include <gtest/gtest.h>

#include <components/mtf.h>

#include <array>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

template <typename UpdateFactory>
void expectRoundTrip(const std::vector<int64_t> &input, UpdateFactory make_update) {
  std::vector<uint64_t> encode_dictionary;
  std::vector<int64_t> encoded = input;
  auto encode_update = make_update();
  moveToFront(encode_dictionary, encoded, encode_update);

  std::vector<uint64_t> decode_dictionary;
  auto decode_update = make_update();
  moveFromFront(decode_dictionary, encoded, decode_update);
  EXPECT_EQ(encoded, input);
}

} // namespace

TEST(Mtf, BaseTransformMatchesKnownVector) {
  std::vector<uint64_t> dictionary;
  std::vector<int64_t> values {0, 1, 0, 2, 1, 0};
  moveToFront(dictionary, values, updateBase);
  EXPECT_EQ(values, (std::vector<int64_t> {0, 1, 1, 2, 2, 2}));
}

TEST(Mtf, BaseTransformMatchesInitializedDictionaryVector) {
  std::vector<uint64_t> dictionary {0, 1, 2, 3};
  std::vector<int64_t> values {2, 2, 1, 0};
  moveToFront(dictionary, values, updateBase);
  EXPECT_EQ(values, (std::vector<int64_t> {2, 0, 2, 2}));
}

TEST(Mtf, HandlesEmptyRepeatedAndGrowingDictionaries) {
  expectRoundTrip({}, [] { return updateBase; });
  expectRoundTrip({5, 5, 5, 5}, [] { return updateBase; });
  expectRoundTrip({0, 300, 1, 299, 300, 0}, [] { return updateBase; });
}

TEST(Mtf, RejectsNegativeSymbolsAndIndexes) {
  std::vector<uint64_t> encode_dictionary;
  std::vector<int64_t> symbols {-1};
  EXPECT_THROW(moveToFront(encode_dictionary, symbols, updateBase), std::invalid_argument);

  std::vector<uint64_t> decode_dictionary;
  std::vector<int64_t> indexes {-1};
  EXPECT_THROW(moveFromFront(decode_dictionary, indexes, updateBase), std::invalid_argument);
}

TEST(Mtf, RejectsDictionaryMissingAnEncodedSymbol) {
  std::vector<uint64_t> dictionary {10};
  std::vector<int64_t> symbols {0};
  auto leave_dictionary_unchanged = [](std::vector<uint64_t> &, size_t) {};
  EXPECT_THROW(
      moveToFront(dictionary, symbols, leave_dictionary_unchanged),
      std::invalid_argument);
}

TEST(Mtf, RoundTripsEveryPreservedUpdateVariant) {
  std::vector<int64_t> input;
  for (int i = 0; i < 200; ++i) {
    input.push_back((i * 37 + 11) % 17);
  }

  expectRoundTrip(input, [] { return updateBase; });
  expectRoundTrip(input, [] { return updateM1ff; });
  expectRoundTrip(input, [] {
    return [previous = std::array<uint64_t, 2> {}](std::vector<uint64_t> &dictionary, size_t index) mutable {
      updateM1ff2(dictionary, index, previous.data());
    };
  });
  expectRoundTrip(input, [] {
    return [](std::vector<uint64_t> &dictionary, size_t index) {
      updateSticky(dictionary, index, 0.5);
    };
  });
  expectRoundTrip(input, [] {
    return [](std::vector<uint64_t> &dictionary, size_t index) {
      update_k(dictionary, index, 2);
    };
  });
  expectRoundTrip(input, [] {
    return [counts = std::map<uint64_t, size_t> {}](std::vector<uint64_t> &dictionary, size_t index) mutable {
      updateC(dictionary, index, 2, counts);
    };
  });
}
