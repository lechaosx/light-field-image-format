#include <gtest/gtest.h>

#include <components/bwt.h>

#include <cstdint>
#include <vector>

namespace {

std::vector<int64_t> roundTrip(std::vector<int64_t> values) {
  const size_t primary_index = bwt(values);
  ibwt(values, primary_index);
  return values;
}

} // namespace

TEST(Bwt, RoundTripsClassicBanana) {
  const std::vector<int64_t> values {'b', 'a', 'n', 'a', 'n', 'a'};
  EXPECT_EQ(roundTrip(values), values);
}

TEST(Bwt, RoundTripsLongAperiodicInput) {
  std::vector<int64_t> values;
  for (int i = 0; i < 199; ++i) {
    values.push_back((i * 37 + 11) % 251);
  }
  EXPECT_EQ(roundTrip(values), values);
}
