#pragma once

#include "components/block.h"
#include "components/dwt.h"

#include <cstdint>
#include <cstddef>

#include <array>
#include <limits>
#include <stdexcept>

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
      block[pos] >>= this->discarded_bits;
    });
  }

  void inversePass(DynamicBlock<int32_t, D> &block) {
    iterate_dimensions<D>(block.size(), [&](const auto &pos) {
      const int64_t value =
          static_cast<int64_t>(block[pos])
          * (int64_t {1} << this->discarded_bits);
      if (value < std::numeric_limits<int32_t>::min()
          || value > std::numeric_limits<int32_t>::max()) {
        throw std::overflow_error("wavelet bit restoration overflow");
      }
      block[pos] = static_cast<int32_t>(value);
    });

    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    idwt<D>(block.size(), proxy);
  }
};
