#include <gtest/gtest.h>

#include <components/bitstream.h>
#include <components/huffman.h>

#include <sstream>
#include <vector>

namespace {

std::vector<HuffmanSymbol> huffman_roundtrip(const std::vector<HuffmanSymbol> &symbols) {
  HuffmanWeights weights;
  for (const auto symbol : symbols) {
    weights[symbol]++;
  }
  const HuffmanEncoder encoder = huff_build(weights);

  std::stringstream table;
  huff_write(encoder, table);
  const HuffmanDecoder decoder = huff_read(table);

  std::stringstream buffer;
  OBitstream out {};
  for (const auto symbol : symbols) {
    huff_encode(encoder, symbol, out, buffer);
  }
  out.flush(buffer);

  IBitstream in {};
  std::vector<HuffmanSymbol> decoded;
  for (size_t i = 0; i < symbols.size(); i++) {
    decoded.push_back(huff_decode(decoder, in, buffer));
  }
  return decoded;
}

} // namespace

TEST(Huffman, RoundTripsSymbolStream) {
  const std::vector<HuffmanSymbol> symbols {1, 1, 1, 2, 2, 3, 4, 4, 4, 4, 5, 7, 7, 42};
  EXPECT_EQ(huffman_roundtrip(symbols), symbols);
}
