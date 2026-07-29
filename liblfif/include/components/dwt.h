/**
* @file dwt.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 22. 1. 2021
* @copyright 2021 Drahomír Dlabaja
* @brief Functions which performs 5/3 FDWT and IDWT.
*/

#pragma once

#include "block.h"
#include "meta.h"

#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>

static inline int64_t shift_right_and_round(int64_t a, unsigned b) {
   return (a + (int64_t {1} << (b - 1))) >> b;
}

template <size_t D>
struct fdwt {
  template <typename F>
  fdwt(const std::array<size_t, D> &block_size, F &&block) {

    std::array<size_t, D - 1> subblock_size {};
    std::copy(std::begin(block_size), std::end(block_size) - 1, std::begin(subblock_size));

    const size_t stride = get_stride<D - 1>(block_size);

    for (size_t slice = 0; slice < block_size[D - 1]; slice++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(slice * stride + index);
      };

      fdwt<D - 1>(subblock_size, subblock);
    }

    for (size_t noodle = 0; noodle < stride; noodle++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(index * stride + noodle);
      };

      fdwt<1>({block_size[D - 1]}, subblock);
    }
  }
};

template <>
struct fdwt<1> {

  template <typename F>
  fdwt(const std::array<size_t, 1> &block_size, F &&block) {
    DynamicBlock<int32_t, 1> inputs({block_size[0]});

    for (size_t x = 0; x < block_size[0]; x++) {
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

    size_t bigger_half = (block_size[0] + 1) >> 1;
    size_t smaller_half = block_size[0] >> 1;

    for (size_t x = 0; x < bigger_half; x++) {
      block(x) = inputs[2 * x];
    }

    for (size_t x = 0; x < smaller_half; x++) {
      block(bigger_half + x) = inputs[2 * x + 1];
    }
  }
};

/**
* @brief Struct for the IDCT which wraps static parameters for partial specialization.
*/
template <size_t D>
struct idwt {

  template <typename F>
  idwt(const std::array<size_t, D> &block_size, F &&block) {

    std::array<size_t, D - 1> subblock_size {};
    std::copy(std::begin(block_size), std::end(block_size) - 1, std::begin(subblock_size));

    const size_t stride = get_stride<D - 1>(block_size);

    for (size_t noodle = 0; noodle < stride; noodle++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(index * stride + noodle);
      };

      idwt<1>({ block_size[D - 1] }, subblock);
    }

    for (size_t slice = 0; slice < block_size[D - 1]; slice++) {
      auto subblock = [&](size_t index) -> auto & {
        return block(slice * stride + index);
      };

      idwt<D - 1>(subblock_size, subblock);
    }
  }
};

template <>
struct idwt<1> {

  template <typename F>
  idwt(const std::array<size_t, 1> &block_size, F &&block) {
    DynamicBlock<int32_t, 1> inputs({block_size[0]});

    size_t bigger_half = (block_size[0] + 1) >> 1;
    size_t smaller_half = block_size[0] >> 1;

    for (size_t x = 0; x < bigger_half; x++) {
      inputs[2 * x] = block(x);
    }

    for (size_t x = 0; x < smaller_half; x++) {
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

    for (size_t x = 0; x < block_size[0]; x++) {
      block(x) = inputs[x];
    }
  }
};
