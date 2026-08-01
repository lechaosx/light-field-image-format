#pragma once

#include "block.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <generator>
#include <numeric>
#include <ranges>

template<size_t Remaining, size_t D>
std::generator<const std::array<size_t, D> &> zigzagScanCore(
    const std::array<size_t, D> &size,
    std::array<size_t, D> &position,
    std::array<size_t, D> &rotation) {
  if constexpr (Remaining == 1) {
    co_yield position;
  }
  else {
    auto move = [&] {
      if (position[rotation[Remaining - 1]]
          < size[rotation[Remaining - 1]] - 1) {
        for (size_t i = 2; i <= Remaining; ++i) {
          if (position[rotation[Remaining - i]] > 0) {
            --position[rotation[Remaining - i]];
            ++position[rotation[Remaining - 1]];
            return true;
          }
        }
      }
      return false;
    };

    do {
      co_yield std::ranges::elements_of(
          zigzagScanCore<Remaining - 1>(size, position, rotation));
    } while (move());

    std::ranges::rotate(
        rotation.begin(),
        rotation.begin() + Remaining - 1,
        rotation.begin() + Remaining);
  }
}

template<size_t D>
std::generator<const std::array<size_t, D> &> zigzagScanStart(
    const std::array<size_t, D> &size,
    std::array<size_t, D> &position) {
  std::array<size_t, D> rotation {};
  std::ranges::iota(rotation, size_t {0});

  auto move = [&] {
    for (size_t i = 0; i < D; ++i) {
      if (position[rotation[i]] < size[rotation[i]] - 1) {
        ++position[rotation[i]];
        return true;
      }
    }
    return false;
  };

  do {
    co_yield std::ranges::elements_of(
        zigzagScanCore<D>(size, position, rotation));
  } while (move());
}

template<size_t D>
std::generator<const std::array<size_t, D> &> zigzagScan(
    std::array<size_t, D> size) {
  std::array<size_t, D> position {};
  co_yield std::ranges::elements_of(zigzagScanStart(size, position));
}

template<size_t D>
std::generator<const std::array<size_t, D> &> zigzagScanSkipFirst(
    std::array<size_t, D> size) {
  std::array<size_t, D> position {};
  position[0] = 1;
  co_yield std::ranges::elements_of(zigzagScanStart(size, position));
}

template<size_t D>
DynamicBlock<size_t, D> zigzagTable(
    const std::array<size_t, D> &size) {
  DynamicBlock<size_t, D> block(size);
  size_t index {};
  for (const auto &position : zigzagScan(size)) {
    block[position] = index++;
  }
  return block;
}
