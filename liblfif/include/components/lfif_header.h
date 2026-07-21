#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>

#include "endian.h"

template <size_t D>
struct LFIFHeader {
  std::array<size_t, D> size;
  std::array<size_t, D> block_size;
  uint8_t depth_bits;
  uint8_t discarded_bits;
  bool predicted;

  void write(std::ostream &output) const {
    writeValueToStream<uint8_t>(depth_bits, output);
    writeValueToStream<uint8_t>(discarded_bits, output);
    writeValueToStream<bool>(predicted, output);

    for (size_t i = 0; i < D; i++) {
      writeValueToStream<uint64_t>(size[i], output);
    }

    for (size_t i = 0; i < D; i++) {
      writeValueToStream<uint64_t>(block_size[i], output);
    }
  }

  void read(std::istream &input) {
    depth_bits     = readValueFromStream<uint8_t>(input);
    discarded_bits = readValueFromStream<uint8_t>(input);
    predicted      = readValueFromStream<bool>(input);

    for (size_t i = 0; i < D; i++) {
      size[i] = readValueFromStream<uint64_t>(input);
    }

    for (size_t i = 0; i < D; i++) {
      block_size[i] = readValueFromStream<uint64_t>(input);
    }
  }
};
