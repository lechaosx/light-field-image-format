#pragma once

#include "block.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>

template<size_t Remaining, size_t D, typename F>
void zigzagScanCore(
    const std::array<size_t, D> &size,
    std::array<size_t, D> &position,
    std::array<size_t, D> &rotation,
    F &callback) {
  if constexpr (Remaining == 1) {
    callback(position);
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
      zigzagScanCore<Remaining - 1>(
          size, position, rotation, callback);
    } while (move());

    std::rotate(
        rotation.begin(),
        rotation.begin() + Remaining - 1,
        rotation.begin() + Remaining);
  }
}

template<size_t D, typename F>
void zigzagScanStart(
    const std::array<size_t, D> &size,
    std::array<size_t, D> position,
    F &&callback) {
  std::array<size_t, D> rotation;
  std::iota(rotation.begin(), rotation.end(), 0);

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
    zigzagScanCore<D>(size, position, rotation, callback);
  } while (move());
}

template<size_t D, typename F>
void zigzagScan(
    const std::array<size_t, D> &size,
    F &&callback) {
  zigzagScanStart(size, std::array<size_t, D> {}, callback);
}

template<size_t D, typename F>
void zigzagScanSkipFirst(
    const std::array<size_t, D> &size,
    F &&callback) {
  std::array<size_t, D> position {};
  position[0] = 1;
  zigzagScanStart(size, position, callback);
}

template<size_t D>
DynamicBlock<size_t, D> zigzagTable(
    const std::array<size_t, D> &size) {
  DynamicBlock<size_t, D> block(size);
  size_t index {};
  zigzagScan(size, [&](const auto &position) {
    block[position] = index++;
  });
  return block;
}
