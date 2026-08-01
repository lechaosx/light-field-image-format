#pragma once

#include "block.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>
#include <vector>

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

template<size_t D>
std::vector<std::array<size_t, D>> zigzagScanStart(
    const std::array<size_t, D> &size,
    std::array<size_t, D> position) {
  std::vector<std::array<size_t, D>> positions;
  size_t position_count = 1;
  for (const size_t extent : size) {
    position_count *= extent;
  }
  positions.reserve(position_count);

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
    auto record = [&](const auto &current) {
      positions.push_back(current);
    };
    zigzagScanCore<D>(size, position, rotation, record);
  } while (move());

  return positions;
}

template<size_t D>
std::vector<std::array<size_t, D>> zigzagScan(
    const std::array<size_t, D> &size) {
  return zigzagScanStart(size, std::array<size_t, D> {});
}

template<size_t D>
std::vector<std::array<size_t, D>> zigzagScanSkipFirst(
    const std::array<size_t, D> &size) {
  std::array<size_t, D> position {};
  position[0] = 1;
  return zigzagScanStart(size, position);
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
