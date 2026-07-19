#pragma once

#include <cmath>
#include <cstdint>
#include <cstddef>
#include <array>

#include "components/block.h"
#include "components/dct.h"
#include "components/meta.h"

template<size_t D>
void dct_forward(DynamicBlock<float, D> &block, const DCTCoefs<D> &coefs, uint8_t discarded_bits) {
  auto proxy = [&](size_t index) -> auto & { return block[index]; };
  fdct<D>(block.size(), coefs, proxy);
  for (const auto &pos : iterate_dimensions<D>(block.size())) {
    block[pos] = std::round(ldexp(block[pos], -discarded_bits));
  }
}

template<size_t D>
void dct_inverse(DynamicBlock<float, D> &block, const DCTCoefs<D> &coefs, uint8_t discarded_bits) {
  for (const auto &pos : iterate_dimensions<D>(block.size())) {
    block[pos] = ldexp(block[pos], discarded_bits);
  }
  auto proxy = [&](size_t index) -> auto & { return block[index]; };
  idct<D>(block.size(), coefs, proxy);
}
