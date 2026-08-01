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

class RunLengthPair {
public:
  size_t zeroes;
  RLAMPUNIT amplitude;

  void huffmanEncodeToStream(
      const HuffmanEncoder &encoder,
      auto &stream,
      size_t class_bits) const {
    encoder.encodeSymbolToStream(huffmanSymbol(class_bits), stream);

    const uint64_t magnitude = amplitude < 0
        ? static_cast<uint64_t>(-(amplitude + 1)) + 1
        : static_cast<uint64_t>(amplitude);
    const uint64_t encoded_amplitude =
        amplitude < 0 ? ~magnitude : magnitude;
    const HuffmanClass amp_class = huffmanClass();
    for (size_t bit = amp_class; bit > 0; --bit) {
      stream.writeBit((encoded_amplitude >> (bit - 1)) & 1);
    }
  }

  void huffmanDecodeFromStream(
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
    const HuffmanClass amp_class = huffman_symbol & class_mask;
    if (amp_class > std::numeric_limits<uint64_t>::digits) {
      throw std::runtime_error("run-length amplitude exceeds codec width");
    }
    zeroes = huffman_symbol >> class_bits;

    uint64_t encoded_amplitude {};
    for (HuffmanClass i = 0; i < amp_class; ++i) {
      encoded_amplitude =
          (encoded_amplitude << 1) | stream.readBit();
    }

    if (amp_class == 0) {
      amplitude = 0;
      return;
    }

    const uint64_t sign_threshold = uint64_t {1} << (amp_class - 1);
    if (encoded_amplitude >= sign_threshold) {
      if (encoded_amplitude
          > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        throw std::runtime_error("run-length amplitude exceeds codec width");
      }
      amplitude = static_cast<int64_t>(encoded_amplitude);
      return;
    }

    const uint64_t amplitude_mask =
        amp_class == std::numeric_limits<uint64_t>::digits
        ? std::numeric_limits<uint64_t>::max()
        : (uint64_t {1} << amp_class) - 1;
    const uint64_t magnitude = amplitude_mask - encoded_amplitude;
    amplitude =
        amp_class == std::numeric_limits<uint64_t>::digits
            && magnitude == sign_threshold
        ? std::numeric_limits<int64_t>::min()
        : -static_cast<int64_t>(magnitude);
  }

  void addToWeights(HuffmanWeights &weights, size_t class_bits) const {
    weights[huffmanSymbol(class_bits)]++;
  }

  [[nodiscard]] bool eob() const {
    return (!zeroes) && (!amplitude);
  }

  [[nodiscard]] static size_t zeroesBits(size_t class_bits) {
    return sizeof(HuffmanSymbol) * 8 - class_bits;
  }

  [[nodiscard]] static size_t classBits(size_t amp_bits) {
    return std::bit_width(amp_bits);
  }

  [[nodiscard]] HuffmanClass huffmanClass() const {
    const uint64_t magnitude = amplitude < 0
        ? static_cast<uint64_t>(-(amplitude + 1)) + 1
        : static_cast<uint64_t>(amplitude);
    return static_cast<HuffmanClass>(std::bit_width(magnitude));
  }

  [[nodiscard]] HuffmanSymbol huffmanSymbol(size_t class_bits) const {
    return zeroes << class_bits | huffmanClass();
  }

};
