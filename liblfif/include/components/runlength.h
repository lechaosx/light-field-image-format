#pragma once

#include "quant_table.h"
#include "huffman.h"
#include "cabac.h"
#include "endian.h"

#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>

using RLAMPUNIT = int64_t;
using HuffmanClass = uint8_t;

struct RunLengthPair {
  size_t zeroes {};
  RLAMPUNIT amplitude {};
};

inline void addRunLengthToWeights(
    const RunLengthPair &pair,
    HuffmanWeights &weights,
    size_t class_bits) {
  const uint64_t magnitude = pair.amplitude < 0
      ? static_cast<uint64_t>(-(pair.amplitude + 1)) + 1
      : static_cast<uint64_t>(pair.amplitude);
  const HuffmanClass amplitude_class =
      static_cast<HuffmanClass>(std::bit_width(magnitude));

  ++weights[static_cast<HuffmanSymbol>(
      pair.zeroes << class_bits | amplitude_class)];
}

inline void encodeRunLength(
    const RunLengthPair &pair,
    const HuffmanEncoder &encoder,
    auto &stream,
    size_t class_bits) {
  const uint64_t magnitude = pair.amplitude < 0
      ? static_cast<uint64_t>(-(pair.amplitude + 1)) + 1
      : static_cast<uint64_t>(pair.amplitude);
  const HuffmanClass amplitude_class =
      static_cast<HuffmanClass>(std::bit_width(magnitude));

  encoder.encodeSymbolToStream(
      static_cast<HuffmanSymbol>(
          pair.zeroes << class_bits | amplitude_class),
      stream);

  const uint64_t encoded_amplitude =
      pair.amplitude < 0 ? ~magnitude : magnitude;
  for (size_t bit = amplitude_class; bit > 0; --bit) {
    stream.writeBit((encoded_amplitude >> (bit - 1)) & 1);
  }
}

[[nodiscard]] inline RunLengthPair decodeRunLength(
    const HuffmanDecoder &decoder,
    auto &stream,
    size_t class_bits) {
  constexpr size_t symbol_bits = std::numeric_limits<HuffmanSymbol>::digits;
  if (class_bits > symbol_bits) {
    throw std::invalid_argument("run-length class width exceeds symbol");
  }
  const HuffmanSymbol class_mask = class_bits == symbol_bits
      ? std::numeric_limits<HuffmanSymbol>::max()
      : static_cast<HuffmanSymbol>((uint32_t {1} << class_bits) - 1);
  const HuffmanSymbol huffman_symbol =
      decoder.decodeSymbolFromStream(stream);
  const HuffmanClass amplitude_class = huffman_symbol & class_mask;
  if (amplitude_class > std::numeric_limits<uint64_t>::digits) {
    throw std::runtime_error("run-length amplitude exceeds codec width");
  }

  RunLengthPair pair {
      .zeroes = static_cast<size_t>(huffman_symbol >> class_bits),
  };
  uint64_t encoded_amplitude {};
  for (HuffmanClass i = 0; i < amplitude_class; ++i) {
    encoded_amplitude =
        (encoded_amplitude << 1) | stream.readBit();
  }

  if (amplitude_class == 0) {
    return pair;
  }

  const uint64_t sign_threshold =
      uint64_t {1} << (amplitude_class - 1);
  if (encoded_amplitude >= sign_threshold) {
    if (encoded_amplitude
        > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
      throw std::runtime_error("run-length amplitude exceeds codec width");
    }
    pair.amplitude = static_cast<int64_t>(encoded_amplitude);
    return pair;
  }

  const uint64_t amplitude_mask =
      amplitude_class == std::numeric_limits<uint64_t>::digits
      ? std::numeric_limits<uint64_t>::max()
      : (uint64_t {1} << amplitude_class) - 1;
  const uint64_t magnitude = amplitude_mask - encoded_amplitude;
  pair.amplitude =
      amplitude_class == std::numeric_limits<uint64_t>::digits
          && magnitude == sign_threshold
      ? std::numeric_limits<int64_t>::min()
      : -static_cast<int64_t>(magnitude);
  return pair;
}
