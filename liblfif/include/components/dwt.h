#pragma once

#include "block.h"
#include "meta.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>

inline int64_t shift_right_and_round(int64_t value, unsigned bits) {
  return (value + (int64_t {1} << (bits - 1))) >> bits;
}

template<size_t D, typename F>
void fdwt(const std::array<size_t, D> &block_size, F &&block) {
  if constexpr (D == 1) {
    DynamicBlock<int32_t, 1> inputs({block_size[0]});

    for (size_t x = 0; x < block_size[0]; ++x) {
      inputs[x] = block(x);
    }

    for (size_t x = 1; x < block_size[0]; x += 2) {
      const int64_t left = inputs[x - 1];
      const int64_t right =
          x >= block_size[0] - 1 ? 0 : inputs[x + 1];
      const int64_t value =
          static_cast<int64_t>(inputs[x])
          - shift_right_and_round(left + right, 1);
      if (value < std::numeric_limits<int32_t>::min()
          || value > std::numeric_limits<int32_t>::max()) {
        throw std::overflow_error("wavelet coefficient overflow");
      }
      inputs[x] = static_cast<int32_t>(value);
    }

    for (size_t x = 0; x < block_size[0]; x += 2) {
      const int64_t left = x ? inputs[x - 1] : 0;
      const int64_t right =
          x >= block_size[0] - 1 ? 0 : inputs[x + 1];
      const int64_t value =
          static_cast<int64_t>(inputs[x])
          + shift_right_and_round(left + right, 2);
      if (value < std::numeric_limits<int32_t>::min()
          || value > std::numeric_limits<int32_t>::max()) {
        throw std::overflow_error("wavelet coefficient overflow");
      }
      inputs[x] = static_cast<int32_t>(value);
    }

    const size_t bigger_half = (block_size[0] + 1) >> 1;
    const size_t smaller_half = block_size[0] >> 1;

    for (size_t x = 0; x < bigger_half; ++x) {
      block(x) = inputs[2 * x];
    }

    for (size_t x = 0; x < smaller_half; ++x) {
      block(bigger_half + x) = inputs[2 * x + 1];
    }
  }
  else {
    std::array<size_t, D - 1> subblock_size;
    std::copy_n(block_size.begin(), D - 1, subblock_size.begin());
    const size_t stride = get_stride<D - 1>(block_size);

    for (size_t slice = 0; slice < block_size[D - 1]; ++slice) {
      fdwt<D - 1>(subblock_size, [&](size_t index) -> auto & {
        return block(slice * stride + index);
      });
    }

    for (size_t noodle = 0; noodle < stride; ++noodle) {
      fdwt<1>(
          std::array<size_t, 1> {block_size[D - 1]},
          [&](size_t index) -> auto & {
            return block(index * stride + noodle);
          });
    }
  }
}

template<size_t D, typename F>
void idwt(const std::array<size_t, D> &block_size, F &&block) {
  if constexpr (D == 1) {
    DynamicBlock<int32_t, 1> inputs({block_size[0]});
    const size_t bigger_half = (block_size[0] + 1) >> 1;
    const size_t smaller_half = block_size[0] >> 1;

    for (size_t x = 0; x < bigger_half; ++x) {
      inputs[2 * x] = block(x);
    }

    for (size_t x = 0; x < smaller_half; ++x) {
      inputs[2 * x + 1] = block(bigger_half + x);
    }

    for (size_t x = 0; x < block_size[0]; x += 2) {
      const int64_t left = x ? inputs[x - 1] : 0;
      const int64_t right =
          x >= block_size[0] - 1 ? 0 : inputs[x + 1];
      const int64_t value =
          static_cast<int64_t>(inputs[x])
          - shift_right_and_round(left + right, 2);
      if (value < std::numeric_limits<int32_t>::min()
          || value > std::numeric_limits<int32_t>::max()) {
        throw std::overflow_error("inverse wavelet overflow");
      }
      inputs[x] = static_cast<int32_t>(value);
    }

    for (size_t x = 1; x < block_size[0]; x += 2) {
      const int64_t left = inputs[x - 1];
      const int64_t right =
          x >= block_size[0] - 1 ? 0 : inputs[x + 1];
      const int64_t value =
          static_cast<int64_t>(inputs[x])
          + shift_right_and_round(left + right, 1);
      if (value < std::numeric_limits<int32_t>::min()
          || value > std::numeric_limits<int32_t>::max()) {
        throw std::overflow_error("inverse wavelet overflow");
      }
      inputs[x] = static_cast<int32_t>(value);
    }

    for (size_t x = 0; x < block_size[0]; ++x) {
      block(x) = inputs[x];
    }
  }
  else {
    std::array<size_t, D - 1> subblock_size;
    std::copy_n(block_size.begin(), D - 1, subblock_size.begin());
    const size_t stride = get_stride<D - 1>(block_size);

    for (size_t noodle = 0; noodle < stride; ++noodle) {
      idwt<1>(
          std::array<size_t, 1> {block_size[D - 1]},
          [&](size_t index) -> auto & {
            return block(index * stride + noodle);
          });
    }

    for (size_t slice = 0; slice < block_size[D - 1]; ++slice) {
      idwt<D - 1>(subblock_size, [&](size_t index) -> auto & {
        return block(slice * stride + index);
      });
    }
  }
}
