/**
* @file huffman.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for Huffman encoding and decoding.
*/

#pragma once

class IBitstream;
class OBitstream;

#include <cstdint>

#include <iosfwd>
#include <vector>
#include <unordered_map>

using HuffmanSymbol     = uint16_t;
using HuffmanWeight     = uint64_t;
using HuffmanCodelength = HuffmanSymbol;
using HuffmanCodeword   = std::vector<bool>;
using HuffmanWeights    = std::unordered_map<HuffmanSymbol, HuffmanWeight>;
using HuffmanMap        = std::unordered_map<HuffmanSymbol, HuffmanCodeword>;

struct HuffmanEncoder {
  std::vector<std::pair<HuffmanWeight, HuffmanSymbol>> codelengths;
  HuffmanMap                                           map;
};

struct HuffmanDecoder {
  std::vector<HuffmanCodelength> counts;
  std::vector<HuffmanSymbol>     symbols;
};

HuffmanEncoder huff_build(const HuffmanWeights &weights);
void           huff_write(const HuffmanEncoder &enc, std::ostream &stream);
HuffmanDecoder huff_read(std::istream &stream);

void                    huff_encode(const HuffmanEncoder &enc, HuffmanSymbol symbol, OBitstream &stream);
[[nodiscard]] HuffmanSymbol huff_decode(const HuffmanDecoder &dec, IBitstream &stream);
