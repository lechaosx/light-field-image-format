/**
* @file runlength.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for performing run-length encoding.
*/

#pragma once

#include <bit>
#include <cstdint>
#include <algorithm>

#include "huffman.h"
#include "cabac.h"
#include "endian.h"

using RLAMPUNIT    = int64_t;
using HuffmanClass = uint8_t;

struct RunLengthPair {
  size_t    zeroes    {};
  RLAMPUNIT amplitude {};
};

inline bool rl_eob(const RunLengthPair &p) {
  return !p.zeroes && !p.amplitude;
}

inline size_t rl_zeroes_bits(size_t class_bits) {
  return sizeof(HuffmanSymbol) * 8 - class_bits;
}

inline size_t rl_class_bits(size_t amp_bits) {
  return std::bit_width(amp_bits);
}

inline HuffmanClass rl_huffman_class(const RunLengthPair &p) {
  uint64_t abs_amp = p.amplitude < 0 ? static_cast<uint64_t>(-p.amplitude)
                                      : static_cast<uint64_t>(p.amplitude);
  return std::bit_width(abs_amp);
}

inline HuffmanSymbol rl_huffman_symbol(const RunLengthPair &p, size_t class_bits) {
  return p.zeroes << class_bits | rl_huffman_class(p);
}

inline void rl_add_to_weights(const RunLengthPair &p, HuffmanWeights &weights, size_t class_bits) {
  weights[rl_huffman_symbol(p, class_bits)]++;
}

void rl_encode(const RunLengthPair &p, const HuffmanEncoder &encoder, OBitstream &stream, size_t class_bits);
void rl_decode(RunLengthPair &p, const HuffmanDecoder &decoder, IBitstream &stream, size_t class_bits);
