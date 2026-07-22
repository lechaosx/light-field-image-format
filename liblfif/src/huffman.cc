/******************************************************************************\
* SOUBOR: huffman.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "components/huffman.h"
#include "components/bitstream.h"
#include "components/endian.h"

#include <algorithm>
#include <numeric>
#include <fstream>

using namespace std;

void HuffmanEncoder::generateFromWeights(const HuffmanWeights &huffman_weights) {
  generateHuffmanCodelengths(huffman_weights);
  generateHuffmanMap();
}

void HuffmanEncoder::writeToStream(ostream &stream) const {
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
  vector<pair<HuffmanWeight, HuffmanSymbol>> A {};

  A.reserve(huffman_weights.size());

  for (auto &pair: huffman_weights) {
    A.push_back({pair.second, pair.first});
  }

  sort(A.begin(), A.end());

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

  sort(A.begin(), A.end());

  m_huffman_codelengths = A;
}

void HuffmanEncoder::generateHuffmanMap() {
  unordered_map<HuffmanSymbol, HuffmanCodeword> map {};

  // TODO PROVE ME

  size_t  prefix_ones      {};
  int64_t huffman_codeword {};

  for (auto &pair: m_huffman_codelengths) {
    map[pair.second]; //DO NOT ERASE, this will emplace a codeword of zero length

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

void HuffmanDecoder::readFromStream(istream &stream) {
  size_t max_codelength = readValueFromStream<HuffmanCodelength>(stream);
  m_huffman_counts.resize(max_codelength + 1);

  for (size_t i = 0; i <= max_codelength; i++) {
    m_huffman_counts[i] = readValueFromStream<HuffmanCodelength>(stream);
  }

  size_t symbols_cnt = accumulate(m_huffman_counts.begin(), m_huffman_counts.end(), 0, plus<size_t>());
  m_huffman_symbols.resize(symbols_cnt);

  for (size_t i = 0; i < symbols_cnt; i++) {
    m_huffman_symbols[i] = readValueFromStream<HuffmanSymbol>(stream);
  }
}

HuffmanSymbol HuffmanDecoder::decodeSymbolFromStream(IBitstream &stream) const {
  return m_huffman_symbols[decodeOneHuffmanSymbolIndex(stream)];
}

size_t HuffmanDecoder::decodeOneHuffmanSymbolIndex(IBitstream &stream) const {
  int64_t code  = 0;
  int64_t first = 0;
  size_t  index = 0;
  HuffmanCodelength count = 0;

  //i genuinely have no idea why this is working

  for (size_t len = 1; len < m_huffman_counts.size(); len++) {
    code |= stream.readBit();
    count = m_huffman_counts[len];
    if (code - count < first) {
      return index + code - first;
    }
    index += count;
    first += count;
    first <<= 1;
    code  <<= 1;
  }

  return 0;
}
