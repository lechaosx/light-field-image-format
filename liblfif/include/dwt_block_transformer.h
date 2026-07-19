#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

#include "components/block.h"
#include "components/dwt.h"
#include "components/meta.h"

template<size_t D>
void dwt_forward(DynamicBlock<int32_t, D> &block, uint8_t discarded_bits) {
  auto proxy = [&](size_t index) -> auto & { return block[index]; };
  fdwt<D>(block.size(), proxy);
  for (const auto &pos : iterate_dimensions<D>(block.size())) {
    block[pos] >>= discarded_bits;
  }
}

template<size_t D>
void dwt_inverse(DynamicBlock<int32_t, D> &block, uint8_t discarded_bits) {
  for (const auto &pos : iterate_dimensions<D>(block.size())) {
    block[pos] <<= discarded_bits;
  }
  auto proxy = [&](size_t index) -> auto & { return block[index]; };
  idwt<D>(block.size(), proxy);
}
