#include <gtest/gtest.h>

#include <components/bitstream.h>
#include <components/huffman.h>
#include <components/runlength.h>

#include <sstream>
#include <utility>
#include <vector>

TEST(RunLength, RoundTripsSignedAmplitudes) {
  const std::vector<RunLengthPair> expected {
    {0, 5}, {3, -12}, {0, 1}, {7, -1}, {15, 100}, {0, -100}, {2, 0}, {0, 0}
  };
  constexpr size_t amplitude_bits = 8;
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
  OBitstream output(stream);
  for (const RunLengthPair &pair : expected) {
    pair.huffmanEncodeToStream(encoder, output, class_bits);
  }
  output.flush();

  IBitstream input(stream);
  for (const RunLengthPair &expected_pair : expected) {
    RunLengthPair decoded {};
    decoded.huffmanDecodeFromStream(decoder, input, class_bits);
    EXPECT_EQ(decoded.zeroes, expected_pair.zeroes);
    EXPECT_EQ(decoded.amplitude, expected_pair.amplitude);
  }
}
