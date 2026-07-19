/**
* @file zigzag.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief The algorithm for generating zig-zag matrices.
*/

#pragma once

#include <algorithm>
#include <array>
#include <generator>
#include <ranges>

#include "block.h"

template <size_t D, size_t N>
std::generator<const std::array<size_t, N>&> zigzagScanCoreGen(
    const std::array<size_t, N> &size, std::array<size_t, N> &pos, std::array<size_t, N> &rot) {
  if constexpr (D == 1) {
    co_yield pos;
  } else {
    auto move = [&]() -> bool {
      if (pos[rot[D - 1]] < size[rot[D - 1]] - 1) {
        for (size_t i = 2; i <= D; i++) {
          if (pos[rot[D - i]] > 0) {
            pos[rot[D - i]]--;
            pos[rot[D - 1]]++;
            return true;
          }
        }
      }
      return false;
    };

    do {
      co_yield std::ranges::elements_of(zigzagScanCoreGen<D - 1, N>(size, pos, rot));
    } while (move());

    std::rotate(rot.begin(), rot.begin() + D - 1, rot.begin() + D);
  }
}

template <size_t D>
std::generator<const std::array<size_t, D>&> zigzagScanStartGen(
    const std::array<size_t, D> &size, std::array<size_t, D> &pos) {
  std::array<size_t, D> rot {};
  std::ranges::iota(rot, size_t{0});

  auto move = [&]() -> bool {
    for (size_t i = 0; i < D; i++) {
      if (pos[rot[i]] < size[rot[i]] - 1) {
        pos[rot[i]]++;
        return true;
      }
    }
    return false;
  };

  do {
    co_yield std::ranges::elements_of(zigzagScanCoreGen<D, D>(size, pos, rot));
  } while (move());
}

template <size_t D>
std::generator<const std::array<size_t, D>&> zigzagScan(std::array<size_t, D> size) {
  std::array<size_t, D> pos {};
  co_yield std::ranges::elements_of(zigzagScanStartGen<D>(size, pos));
}

template <size_t D>
std::generator<const std::array<size_t, D>&> zigzagScanSkipFirst(std::array<size_t, D> size) {
  std::array<size_t, D> pos {};
  pos[0] = 1;
  co_yield std::ranges::elements_of(zigzagScanStartGen<D>(size, pos));
}

template <size_t D>
DynamicBlock<size_t, D> zigzagTable(const std::array<size_t, D> &size) {
  DynamicBlock<size_t, D> block(size);
  size_t i = 0;
  for (const auto &pos : zigzagScan<D>(size)) {
    block[pos] = i++;
  }
  return block;
}
