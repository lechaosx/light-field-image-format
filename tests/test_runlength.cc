#include <gtest/gtest.h>

#include <components/bitstream.h>
#include <components/huffman.h>
#include <components/runlength.h>

#include "bitstream_io.h"

#include <cstdint>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>

TEST(RunLength, RoundTripsSignedAmplitudes) {
  const std::vector<RunLengthPair> expected {
    {0, 5},
    {3, -12},
    {0, 1},
    {7, -1},
    {15, 100},
    {0, -100},
    {2, std::numeric_limits<int32_t>::max()},
    {1, -static_cast<int64_t>(std::numeric_limits<int32_t>::max())},
    {0, std::numeric_limits<int64_t>::max()},
    {0, std::numeric_limits<int64_t>::min()},
    {2, 0},
    {0, 0},
  };
  constexpr size_t amplitude_bits = 64;
  const size_t class_bits = RunLengthPair::classBits(amplitude_bits);

  HuffmanWeights weights;
  for (const RunLengthPair &pair : expected) {
    pair.addToWeights(weights, class_bits);
  }

  HuffmanEncoder encoder;
  encoder.generateFromWeights(weights);
  std::stringstream table;
  encoder.writeToStream(table);

  HuffmanDecoder decoder;
  decoder.readFromStream(table);

  std::stringstream stream;
  OBitstream output(byteSink(stream));
  for (const RunLengthPair &pair : expected) {
    pair.huffmanEncodeToStream(encoder, output, class_bits);
  }
  output.flush();

  IBitstream input(byteSource(stream));
  for (const RunLengthPair &expected_pair : expected) {
    RunLengthPair decoded {};
    decoded.huffmanDecodeFromStream(decoder, input, class_bits);
    EXPECT_EQ(decoded.zeroes, expected_pair.zeroes);
    EXPECT_EQ(decoded.amplitude, expected_pair.amplitude);
  }
}
