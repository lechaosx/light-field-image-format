#include <gtest/gtest.h>

#include <components/bwt.h>

#include <vector>
#include <cstdint>

namespace {

std::vector<int64_t> bwt_roundtrip(std::vector<int64_t> string) {
  std::vector<int64_t> transformed = string;
  const size_t primary_index = bwt(transformed);
  ibwt(transformed, primary_index);
  return transformed;
}

} // namespace

TEST(Bwt, RoundTripsClassicBanana) {
  const std::vector<int64_t> string {'b', 'a', 'n', 'a', 'n', 'a'};
  EXPECT_EQ(bwt_roundtrip(string), string);
}

TEST(Bwt, RoundTripsLongVariedData) {
  // Distinct values (37 is coprime to 251) => distinct rotations, so the
  // primary index alone recovers the string. A periodic input could only be
  // recovered up to rotation, which is an inherent BWT property, not a bug.
  std::vector<int64_t> string;
  for (int i = 0; i < 199; i++) {
    string.push_back((i * 37 + 11) % 251);
  }
  EXPECT_EQ(bwt_roundtrip(string), string);
}
