/**
* @file diag_scan.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 27. 1. 2021
* @copyright 2021 Drahomír Dlabaja
* @brief The algorithm for scan by diagonal slices from DC.
*/

#pragma once

#include "block.h"

#include <cstdint>
#include <vector>
#include <array>
#include <numeric>
#include <algorithm>

template <size_t D, typename F>
void diagonalScanCore(const size_t size[D], size_t pos[D], F &&callback) {
  if constexpr (D == 1) {
    callback();
  } else {
    size_t starting_pos[D];
    std::copy(pos, pos + D, starting_pos);

    auto move = [&]() {
      if (pos[D - 1] + 1 < size[D - 1]) {
        for (size_t i = 2; i <= D; i++) {
          if (pos[D - i] > 0) {
            pos[D - i]--;
            pos[D - 1]++;
            return true;
          }
        }
      }

      return false;
    };

    do {
      diagonalScanCore<D - 1>(size, pos, callback);
    } while (move());

    std::copy(starting_pos, starting_pos + D, pos);
  }
}

template <size_t D, typename F>
void diagonalScan(const std::array<size_t, D> &size, size_t diag, F &&callback) {
  std::array<size_t, D> pos {};

  for (size_t i = 0 ; i < D; i++) {
    size_t min = std::min(diag, size[i] - 1);
    pos[i] = min;
    diag -= min;
  }

  auto return_pos = [&]() {
    callback(pos);
  };

  diagonalScanCore<D>(size.data(), pos.data(), return_pos);
}
