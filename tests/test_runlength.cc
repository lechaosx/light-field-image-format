#include <gtest/gtest.h>

#include <components/bitstream.h>
#include <components/huffman.h>
#include <components/runlength.h>

#include <sstream>
#include <utility>
#include <vector>

namespace {

// RunLengthPair as a comparable value so the test body can assert with EXPECT_EQ.
using Pair = std::pair<size_t, RLAMPUNIT>;

std::vector<Pair> runlength_roundtrip(const std::vector<RunLengthPair> &pairs, size_t amp_bits) {
  const size_t class_bits = rl_class_bits(amp_bits);

  HuffmanWeights weights;
  for (const auto &pair : pairs) {
    rl_add_to_weights(pair, weights, class_bits);
  }
  const HuffmanEncoder encoder = huff_build(weights);

  std::stringstream table;
  huff_write(encoder, table);
  const HuffmanDecoder decoder = huff_read(table);

  std::stringstream buffer;
  OBitstream out {};
  for (const auto &pair : pairs) {
    rl_encode(pair, encoder, out, buffer, class_bits);
  }
  out.flush(buffer);

  IBitstream in {};
  std::vector<Pair> decoded;
  for (size_t i = 0; i < pairs.size(); i++) {
    RunLengthPair pair {};
    rl_decode(pair, decoder, in, buffer, class_bits);
    decoded.emplace_back(pair.zeroes, pair.amplitude);
  }
  return decoded;
}

} // namespace

TEST(RunLength, RoundTripsRunsWithSignedAmplitudes) {
  const std::vector<RunLengthPair> pairs {
    {0, 5}, {3, -12}, {0, 1}, {7, -1}, {15, 100}, {0, -100}, {2, 0}, {0, 0},
  };
  const std::vector<Pair> expected {
    {0, 5}, {3, -12}, {0, 1}, {7, -1}, {15, 100}, {0, -100}, {2, 0}, {0, 0},
  };
  EXPECT_EQ(runlength_roundtrip(pairs, 8), expected);
}
