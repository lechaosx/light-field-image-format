#pragma once

#include "components/block.h"
#include "components/dct.h"

#include <cstdint>
#include <cstddef>

#include <array>

template<size_t D>
void dctForward(
    DynamicBlock<float, D> &block,
    const DCTCoefs<D> &dct_coefs,
    uint8_t discarded_bits) {
  auto proxy = [&](size_t index) -> auto & {
    return block[index];
  };
  fdct<D>(block.size(), dct_coefs, proxy);
  for (const auto &pos : iterate_dimensions<D>(block.size())) {
    block[pos] = std::round(ldexp(block[pos], -discarded_bits));
  }
}

template<size_t D>
void dctInverse(
    DynamicBlock<float, D> &block,
    const DCTCoefs<D> &dct_coefs,
    uint8_t discarded_bits) {
  for (const auto &pos : iterate_dimensions<D>(block.size())) {
    block[pos] = ldexp(block[pos], discarded_bits);
  }
  auto proxy = [&](size_t index) -> auto & {
    return block[index];
  };
  idct<D>(block.size(), dct_coefs, proxy);
}
