/**
* @file dwt.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 22. 1. 2021
* @copyright 2021 Drahomír Dlabaja
* @brief Functions which performs 5/3 FDWT and IDWT.
*/

#pragma once

#include <cmath>
#include <array>

#include "block.h"
#include "meta.h"

static constexpr int shift_right_and_round(int32_t a, int32_t b) {
   return (a + (1 << (b - 1))) >> b;
}

template <size_t D, LinearRef<int32_t> Block>
void fdwt(const std::array<size_t, D> &block_size, Block &&block) {
  if constexpr (D == 1) {
    DynamicBlock<int32_t, 1> inputs({block_size[0]});

    for (size_t x = 0; x < block_size[0]; x++) {
      inputs[x] = block(x);
    }

    for (size_t x = 1; x < block_size[0]; x += 2) {
      auto left  = inputs[x - 1];
      auto right = x >= block_size[0] - 1 ? int32_t{0} : inputs[x + 1];
      inputs[x] -= shift_right_and_round(left + right, 1);
    }

    for (size_t x = 0; x < block_size[0]; x += 2) {
      auto left  = x ? inputs[x - 1] : int32_t{0};
      auto right = x >= block_size[0] - 1 ? int32_t{0} : inputs[x + 1];
      inputs[x] += shift_right_and_round(left + right, 2);
    }

    const auto bigger_half  = (block_size[0] + 1) >> 1;
    const auto smaller_half = block_size[0] >> 1;

    for (size_t x = 0; x < bigger_half; x++) {
      block(x) = inputs[2 * x];
    }

    for (size_t x = 0; x < smaller_half; x++) {
      block(bigger_half + x) = inputs[2 * x + 1];
    }
  } else {
    std::array<size_t, D - 1> subblock_size {};
    std::copy(std::begin(block_size), std::end(block_size) - 1, std::begin(subblock_size));

    const auto stride = get_stride<D - 1>(block_size);

    for (size_t slice = 0; slice < block_size[D - 1]; slice++) {
      fdwt<D - 1>(subblock_size, [&](size_t index) -> auto & {
        return block(slice * stride + index);
      });
    }

    for (size_t noodle = 0; noodle < stride; noodle++) {
      fdwt<1>(std::array<size_t, 1>{block_size[D - 1]}, [&](size_t index) -> auto & {
        return block(index * stride + noodle);
      });
    }
  }
}

template <size_t D, LinearRef<int32_t> Block>
void idwt(const std::array<size_t, D> &block_size, Block &&block) {
  if constexpr (D == 1) {
    DynamicBlock<int32_t, 1> inputs({block_size[0]});

    const auto bigger_half  = (block_size[0] + 1) >> 1;
    const auto smaller_half = block_size[0] >> 1;

    for (size_t x = 0; x < bigger_half; x++) {
      inputs[2 * x] = block(x);
    }

    for (size_t x = 0; x < smaller_half; x++) {
      inputs[2 * x + 1] = block(bigger_half + x);
    }

    for (size_t x = 0; x < block_size[0]; x += 2) {
      auto left  = x ? inputs[x - 1] : int32_t{0};
      auto right = x >= block_size[0] - 1 ? int32_t{0} : inputs[x + 1];
      inputs[x] -= shift_right_and_round(left + right, 2);
    }

    for (size_t x = 1; x < block_size[0]; x += 2) {
      auto left  = inputs[x - 1];
      auto right = x >= block_size[0] - 1 ? int32_t{0} : inputs[x + 1];
      inputs[x] += shift_right_and_round(left + right, 1);
    }

    for (size_t x = 0; x < block_size[0]; x++) {
      block(x) = inputs[x];
    }
  } else {
    std::array<size_t, D - 1> subblock_size {};
    std::copy(std::begin(block_size), std::end(block_size) - 1, std::begin(subblock_size));

    const auto stride = get_stride<D - 1>(block_size);

    for (size_t noodle = 0; noodle < stride; noodle++) {
      idwt<1>(std::array<size_t, 1>{block_size[D - 1]}, [&](size_t index) -> auto & {
        return block(index * stride + noodle);
      });
    }

    for (size_t slice = 0; slice < block_size[D - 1]; slice++) {
      idwt<D - 1>(subblock_size, [&](size_t index) -> auto & {
        return block(slice * stride + index);
      });
    }
  }
}
