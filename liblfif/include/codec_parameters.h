#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

template <size_t D>
void validateCodecParameters(
    const std::array<size_t, D> &size,
    const std::array<size_t, D> &block_size,
    uint8_t depth_bits) {
  if (depth_bits == 0 || depth_bits > 16) {
    throw std::invalid_argument("sample depth must be between 1 and 16 bits");
  }

  size_t image_values = 1;
  size_t block_values = 1;
  size_t aligned_values = 1;
  for (size_t i = 0; i < D; ++i) {
    if (size[i] == 0) {
      throw std::invalid_argument("image extents must be nonzero");
    }
    if (block_size[i] == 0) {
      throw std::invalid_argument("block extents must be nonzero");
    }

    const size_t blocks = size[i] / block_size[i] + (size[i] % block_size[i] != 0);
    if (blocks > std::numeric_limits<size_t>::max() / block_size[i]) {
      throw std::length_error("aligned image extents overflow");
    }
    const size_t aligned_extent = blocks * block_size[i];

    if (image_values > std::numeric_limits<size_t>::max() / size[i]
        || block_values > std::numeric_limits<size_t>::max() / block_size[i]
        || aligned_values > std::numeric_limits<size_t>::max() / aligned_extent) {
      throw std::length_error("codec dimensions overflow");
    }
    image_values *= size[i];
    block_values *= block_size[i];
    aligned_values *= aligned_extent;
  }
}
