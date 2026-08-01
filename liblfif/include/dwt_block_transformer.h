#pragma once

#include "components/block.h"
#include "components/dwt.h"

#include <cstdint>
#include <cstddef>

#include <array>
#include <limits>
#include <stdexcept>

template<size_t D>
void dwtForward(DynamicBlock<int32_t, D> &block, uint8_t discarded_bits) {
  auto proxy = [&](size_t index) -> auto & {
    return block[index];
  };
  fdwt<D>(block.size(), proxy);
  for (const auto &pos : iterate_dimensions<D>(block.size())) {
    block[pos] >>= discarded_bits;
  }
}

template<size_t D>
void dwtInverse(DynamicBlock<int32_t, D> &block, uint8_t discarded_bits) {
  for (const auto &pos : iterate_dimensions<D>(block.size())) {
    const int64_t value =
        static_cast<int64_t>(block[pos])
        * (int64_t {1} << discarded_bits);
    if (value < std::numeric_limits<int32_t>::min()
        || value > std::numeric_limits<int32_t>::max()) {
      throw std::overflow_error("wavelet bit restoration overflow");
    }
    block[pos] = static_cast<int32_t>(value);
  }
  auto proxy = [&](size_t index) -> auto & {
    return block[index];
  };
  idwt<D>(block.size(), proxy);
}
