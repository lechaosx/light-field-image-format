#pragma once

#include <cstdint>

#include <iosfwd>
#include <stdexcept>
#include <unordered_map>
#include <vector>

using HuffmanSymbol = uint16_t;
using HuffmanWeight = uint64_t;
using HuffmanCodelength = HuffmanSymbol;
using HuffmanCodeword = std::vector<bool>;

using HuffmanWeights     = std::unordered_map<HuffmanSymbol, HuffmanWeight>;

using HuffmanMap         = std::unordered_map<HuffmanSymbol, HuffmanCodeword>;

class HuffmanEncoder {
public:

  void generateFromWeights(const HuffmanWeights &huffman_weights);

  void writeToStream(std::ostream &stream) const;

  void encodeSymbolToStream(
      const HuffmanSymbol symbol,
      auto &stream) const {
    stream.write(m_huffman_map.at(symbol));
  }

private:
  void generateHuffmanCodelengths(const HuffmanWeights &huffman_weights);
  void generateHuffmanMap();

  std::vector<std::pair<HuffmanWeight, HuffmanSymbol>> m_huffman_codelengths;
  HuffmanMap                                           m_huffman_map;
};

class HuffmanDecoder {
public:

  void readFromStream(std::istream &stream);

  HuffmanSymbol decodeSymbolFromStream(auto &stream) const {
    const size_t index = decodeOneHuffmanSymbolIndex(stream);
    if (index >= m_huffman_symbols.size()) {
      throw std::runtime_error("invalid Huffman symbol index");
    }
    return m_huffman_symbols[index];
  }

private:
  size_t decodeOneHuffmanSymbolIndex(auto &stream) const {
    if (m_huffman_counts.size() == 1 && m_huffman_counts[0] == 1) {
      return 0;
    }

    uint64_t code {};
    uint64_t first {};
    size_t index {};

    for (size_t length = 1; length < m_huffman_counts.size(); ++length) {
      code |= stream.readBit();
      const HuffmanCodelength count = m_huffman_counts[length];
      if (code >= first && code - first < count) {
        return index + static_cast<size_t>(code - first);
      }
      index += count;
      first = (first + count) << 1;
      code <<= 1;
    }

    throw std::runtime_error("invalid Huffman codeword");
  }

  std::vector<HuffmanCodelength> m_huffman_counts;
  std::vector<HuffmanSymbol>     m_huffman_symbols;
};
