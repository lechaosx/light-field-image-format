/**
* @file diag_scan.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 27. 1. 2021
* @copyright 2021 Drahomír Dlabaja
* @brief The algorithm for scan by diagonal slices from DC.
*/

#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <array>
#include <numeric>
#include <algorithm>

#include "block.h"
#include "meta.h"

template <size_t D, typename F>
void diagonalScanCore(std::span<const size_t, D> size, std::span<size_t, D> pos, F &&callback) {
  if constexpr (D == 1) {
    callback();
  } else {
    std::array<size_t, D> starting_pos;
    std::copy(pos.begin(), pos.end(), starting_pos.begin());

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
      diagonalScanCore<D - 1>(size.template first<D - 1>(), pos.template first<D - 1>(), callback);
    } while (move());

    std::copy(starting_pos.begin(), starting_pos.end(), pos.begin());
  }
}

template <size_t D, DimCallback<D> F>
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

  diagonalScanCore(std::span<const size_t, D>{size}, std::span<size_t, D>{pos}, return_pos);
}
