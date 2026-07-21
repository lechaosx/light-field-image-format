#include <algorithm>
#include <numeric>
#include <fstream>

#include <components/huffman.h>
#include <components/bitstream.h>
#include <components/endian.h>

static std::vector<std::pair<HuffmanWeight, HuffmanSymbol>>
build_codelengths(const HuffmanWeights &huffman_weights) {
  std::vector<std::pair<HuffmanWeight, HuffmanSymbol>> A {};

  A.reserve(huffman_weights.size());

  for (auto &pair: huffman_weights) {
    A.push_back({pair.second, pair.first});
  }

  std::ranges::sort(A);

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

  std::ranges::sort(A);
  return A;
}

static HuffmanMap build_map(const std::vector<std::pair<HuffmanWeight, HuffmanSymbol>> &codelengths) {
  HuffmanMap map {};

  // TODO PROVE ME

  size_t  prefix_ones      {};
  int64_t huffman_codeword {};

  for (auto &pair: codelengths) {
    map[pair.second];

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

  return map;
}

HuffmanEncoder huff_build(const HuffmanWeights &weights) {
  HuffmanEncoder enc {};
  enc.codelengths = build_codelengths(weights);
  enc.map         = build_map(enc.codelengths);
  return enc;
}

void huff_write(const HuffmanEncoder &enc, std::ostream &stream) {
  HuffmanCodelength max_codelength = enc.codelengths.back().first;
  writeValueToStream(max_codelength, stream);

  auto it = enc.codelengths.begin();
  for (size_t i = 0; i <= max_codelength; i++) {
    HuffmanCodelength leaves = 0;
    while ((it < enc.codelengths.end()) && (it->first == i)) {
      leaves++;
      it++;
    }
    writeValueToStream(leaves, stream);
  }

  for (auto &pair: enc.codelengths) {
    writeValueToStream(pair.second, stream);
  }
}

HuffmanDecoder huff_read(std::istream &stream) {
  HuffmanDecoder dec {};

  size_t max_codelength = readValueFromStream<HuffmanCodelength>(stream);
  dec.counts.resize(max_codelength + 1);

  for (size_t i = 0; i <= max_codelength; i++) {
    dec.counts[i] = readValueFromStream<HuffmanCodelength>(stream);
  }

  size_t symbols_cnt = std::accumulate(dec.counts.begin(), dec.counts.end(), size_t{0});
  dec.symbols.resize(symbols_cnt);

  for (size_t i = 0; i < symbols_cnt; i++) {
    dec.symbols[i] = readValueFromStream<HuffmanSymbol>(stream);
  }

  return dec;
}

void huff_encode(const HuffmanEncoder &enc, HuffmanSymbol symbol, OBitstream &bitstream, std::ostream &stream) {
  bitstream.write(stream, enc.map.at(symbol));
}

HuffmanSymbol huff_decode(const HuffmanDecoder &dec, IBitstream &bitstream, std::istream &stream) {
  int64_t code  = 0;
  int64_t first = 0;
  size_t  index = 0;
  HuffmanCodelength count = 0;

  //i genuinely have no idea why this is working

  for (size_t len = 1; len < dec.counts.size(); len++) {
    code |= bitstream.readBit(stream);
    count = dec.counts[len];
    if (code - count < first) {
      return dec.symbols[index + code - first];
    }
    index += count;
    first += count;
    first <<= 1;
    code  <<= 1;
  }

  return 0;
}
