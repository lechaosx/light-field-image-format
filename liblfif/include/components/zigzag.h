/**
* @file zigzag.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief The algorithm for generating zig-zag matrices.
*/

#pragma once

#include <array>
#include <algorithm>
#include <numeric>

#include "block.h"

template <size_t D, size_t N, typename F>
void zigzagScanCore(const std::array<size_t, N> &size, std::array<size_t, N> &pos, std::array<size_t, N> &rot, F &&callback) {
  if constexpr (D == 1) {
    callback(pos);
  } else {
    auto move = [&]() {
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
      zigzagScanCore<D - 1, N>(size, pos, rot, callback);
    } while (move());

    std::rotate(rot.begin(), rot.begin() + D - 1, rot.begin() + D);
  }
}

template <size_t D, typename F>
void zigzagScanStart(const std::array<size_t, D> &size, std::array<size_t, D> &pos, F &&callback) {
  std::array<size_t, D> rot {};
  std::iota(rot.begin(), rot.end(), size_t{0});

  auto move = [&]() {
    for (size_t i = 0 ; i < D; i++) {
      if (pos[rot[i]] < size[rot[i]] - 1) {
        pos[rot[i]]++;
        return true;
      }
    }
    return false;
  };

  do {
    zigzagScanCore<D, D>(size, pos, rot, callback);
  } while (move());
}

template <size_t D, typename F>
void zigzagScan(const std::array<size_t, D> &size, F &&callback) {
  std::array<size_t, D> pos {};
  zigzagScanStart<D>(size, pos, callback);
}

template <size_t D, typename F>
void zigzagScanSkipFirst(const std::array<size_t, D> &size, F &&callback) {
  std::array<size_t, D> pos {};
  pos[0] = 1;
  zigzagScanStart<D>(size, pos, callback);
}

template <size_t D>
DynamicBlock<size_t, D> zigzagTable(const std::array<size_t, D> &size) {
  DynamicBlock<size_t, D> block(size);
  size_t i = 0;
  zigzagScan<D>(size, [&](const std::array<size_t, D> &pos) {
    block[pos] = i++;
  });
  return block;
}
