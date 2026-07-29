#include <gtest/gtest.h>

#include <components/bitstream.h>
#include <components/endian.h>
#include <components/huffman.h>

#include <sstream>
#include <vector>

TEST(Huffman, RoundTripsSymbolStream) {
  const std::vector<HuffmanSymbol> expected {1, 1, 1, 2, 2, 3, 4, 4, 4, 4, 5, 7, 7, 42};

  HuffmanWeights weights;
  for (const HuffmanSymbol symbol : expected) {
    ++weights[symbol];
  }

  HuffmanEncoder encoder;
  encoder.generateFromWeights(weights);
  std::stringstream table;
  encoder.writeToStream(table);

  HuffmanDecoder decoder;
  decoder.readFromStream(table);

  std::stringstream stream;
  OBitstream output(stream);
  for (const HuffmanSymbol symbol : expected) {
    encoder.encodeSymbolToStream(symbol, output);
  }
  output.flush();

  IBitstream input(stream);
  std::vector<HuffmanSymbol> decoded;
  for (size_t i = 0; i < expected.size(); ++i) {
    decoded.push_back(decoder.decodeSymbolFromStream(input));
  }
  EXPECT_EQ(decoded, expected);
}

TEST(Huffman, RejectsUnmatchedCodeword) {
  std::stringstream table;
  writeValueToStream<HuffmanCodelength>(1, table);
  writeValueToStream<HuffmanCodelength>(0, table);
  writeValueToStream<HuffmanCodelength>(1, table);
  writeValueToStream<HuffmanSymbol>(42, table);

  HuffmanDecoder decoder;
  decoder.readFromStream(table);

  std::istringstream encoded(std::string(1, '\1'));
  IBitstream input(encoded);
  EXPECT_THROW(decoder.decodeSymbolFromStream(input), std::runtime_error);
}
