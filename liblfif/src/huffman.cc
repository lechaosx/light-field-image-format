/******************************************************************************\
* SOUBOR: huffman.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "components/huffman.h"
#include "components/bitstream.h"
#include "components/endian.h"

#include <algorithm>
#include <istream>
#include <limits>
#include <ostream>
#include <stdexcept>


void HuffmanEncoder::generateFromWeights(const HuffmanWeights &huffman_weights) {
  generateHuffmanCodelengths(huffman_weights);
  generateHuffmanMap();
}

void HuffmanEncoder::writeToStream(std::ostream &stream) const {
  HuffmanCodelength max_codelength = m_huffman_codelengths.back().first;
  writeValueToStream(max_codelength, stream);

  auto it = m_huffman_codelengths.begin();
  for (size_t i = 0; i <= max_codelength; i++) {
    HuffmanCodelength leaves = 0;
    while ((it < m_huffman_codelengths.end()) && (it->first == i)) {
      leaves++;
      it++;
    }
    writeValueToStream(leaves, stream);
  }

  for (auto &pair: m_huffman_codelengths) {
    writeValueToStream(pair.second, stream);
  }
}

void HuffmanEncoder::encodeSymbolToStream(HuffmanSymbol symbol, OBitstream &stream) const {
  stream.write(m_huffman_map.at(symbol));
}

void HuffmanEncoder::generateHuffmanCodelengths(const HuffmanWeights &huffman_weights) {
  std::vector<std::pair<HuffmanWeight, HuffmanSymbol>> A {};

  A.reserve(huffman_weights.size());

  for (auto &pair: huffman_weights) {
    A.push_back({pair.second, pair.first});
  }

  std::sort(A.begin(), A.end());

  // SOURCE: http://hjemmesider.diku.dk/~jyrki/Paper/WADS95.pdf

  size_t n = A.size();

  uint64_t s = 1;
  uint64_t r = 1;

  for (uint64_t t = 1; t <= n - 1; t++) {
    uint64_t sum = 0;
    for (size_t i = 0; i < 2; i++) {
      if ((s > n) || ((r < t) && (A[r-1].first < A[s-1].first))) {
        sum += A[r-1].first;
        A[r-1].first = t;
        r++;
      }
      else {
        sum += A[s-1].first;
        s++;
      }
    }
    A[t-1].first = sum;
  }

  if (n >= 2) {
    A[n - 2].first = 0;
  }

  for (int64_t t = n - 2; t >= 1; t--) {
    A[t-1].first = A[A[t-1].first-1].first + 1;
  }

  int64_t a  = 1;
  int64_t u  = 0;
  uint64_t d = 0;
  uint64_t t = n - 1;
  uint64_t x = n;

  while (a > 0) {
    while ((t >= 1) && (A[t-1].first == d)) {
      u++;
      t--;
    }
    while (a > u) {
      A[x-1].first = d;
      x--;
      a--;
    }
    a = 2 * u;
    d++;
    u = 0;
  }

  std::sort(A.begin(), A.end());

  m_huffman_codelengths = A;
}

void HuffmanEncoder::generateHuffmanMap() {
  std::unordered_map<HuffmanSymbol, HuffmanCodeword> map {};

  size_t  prefix_ones      {};
  int64_t huffman_codeword {};

  for (auto &pair: m_huffman_codelengths) {
    // A single-symbol alphabet has a zero-length codeword.
    map.try_emplace(pair.second);

    for (size_t i = 0; i < prefix_ones; i++) {
      map[pair.second].push_back(1);
    }

    size_t len = pair.first - prefix_ones;

    for (size_t k = 0; k < len; k++) {
      map[pair.second].push_back((huffman_codeword >> (63 - k)) & 1);
    }

    huffman_codeword = ((huffman_codeword >> (64 - len)) + 1) << (64 - len);
    while (huffman_codeword < 0) {
      prefix_ones++;
      huffman_codeword <<= 1;
    }
  }

  m_huffman_map = map;
}

void HuffmanDecoder::readFromStream(std::istream &stream) {
  const size_t max_codelength = readValueFromStream<HuffmanCodelength>(stream);
  if (max_codelength > 64) {
    throw std::length_error("Huffman code length exceeds decoder width");
  }
  std::vector<HuffmanCodelength> counts(max_codelength + 1);

  for (size_t i = 0; i <= max_codelength; i++) {
    counts[i] = readValueFromStream<HuffmanCodelength>(stream);
  }

  constexpr size_t alphabet_size =
      static_cast<size_t>(std::numeric_limits<HuffmanSymbol>::max()) + 1;
  size_t symbols_cnt = 0;
  for (const HuffmanCodelength count : counts) {
    if (count > alphabet_size - symbols_cnt) {
      throw std::length_error("Huffman alphabet exceeds symbol type");
    }
    symbols_cnt += count;
  }
  if (symbols_cnt == 0
      || (counts[0] != 0 && (counts[0] != 1 || symbols_cnt != 1))) {
    throw std::runtime_error("invalid zero-length Huffman code");
  }

  uint64_t available = 1;
  for (size_t length = 1; length < counts.size(); ++length) {
    available = available > std::numeric_limits<uint64_t>::max() / 2
        ? std::numeric_limits<uint64_t>::max()
        : available * 2;
    if (counts[length] > available) {
      throw std::runtime_error("oversubscribed Huffman code lengths");
    }
    available -= counts[length];
  }

  std::vector<HuffmanSymbol> symbols(symbols_cnt);

  for (size_t i = 0; i < symbols_cnt; i++) {
    symbols[i] = readValueFromStream<HuffmanSymbol>(stream);
  }
  m_huffman_counts = std::move(counts);
  m_huffman_symbols = std::move(symbols);
}

HuffmanSymbol HuffmanDecoder::decodeSymbolFromStream(IBitstream &stream) const {
  const size_t index = decodeOneHuffmanSymbolIndex(stream);
  if (index >= m_huffman_symbols.size()) {
    throw std::runtime_error("invalid Huffman symbol index");
  }
  return m_huffman_symbols[index];
}

size_t HuffmanDecoder::decodeOneHuffmanSymbolIndex(IBitstream &stream) const {
  if (m_huffman_counts.size() == 1 && m_huffman_counts[0] == 1) {
    return 0;
  }

  uint64_t code  = 0;
  uint64_t first = 0;
  size_t  index = 0;
  HuffmanCodelength count = 0;

  for (size_t len = 1; len < m_huffman_counts.size(); len++) {
    code |= stream.readBit();
    count = m_huffman_counts[len];
    if (code >= first && code - first < count) {
      return index + static_cast<size_t>(code - first);
    }
    index += count;
    first += count;
    first <<= 1;
    code  <<= 1;
  }

  throw std::runtime_error("invalid Huffman codeword");
}
