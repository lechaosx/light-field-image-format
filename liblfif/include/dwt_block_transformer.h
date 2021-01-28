#pragma once

#include "components/block.h"
#include "components/dwt.h"

#include <cstdint>
#include <cstddef>

#include <array>

template<size_t D>
class DWTBlockTransformer {
  uint8_t discarded_bits;

public:

  DWTBlockTransformer(uint8_t discarded_bits) {
    this->discarded_bits = discarded_bits;
  }

  void forwardPass(DynamicBlock<int32_t, D> &block) {
    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    fdwt<D>(block.size(), proxy);

    iterate_dimensions<D>(block.size(), [&](const auto &pos) {
      block[pos] >>= this->discarded_bits; //QUANTIZATION
    });
  }

  void inversePass(DynamicBlock<int32_t, D> &block) {
    iterate_dimensions<D>(block.size(), [&](const auto &pos) {
      block[pos] <<= this->discarded_bits;
    });

    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    idwt<D>(block.size(), proxy);
  }
};
