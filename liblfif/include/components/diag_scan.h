/**
* @file diag_scan.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 27. 1. 2021
* @copyright 2021 Drahomír Dlabaja
* @brief The algorithm for scan by diagonal slices from DC.
*/

#pragma once

#include <array>
#include <generator>
#include <ranges>
#include <span>

template <size_t D, size_t N>
std::generator<const std::array<size_t, N>&> diagonalScanCoreGen(
    std::span<const size_t, D> size,
    std::span<size_t, D> pos,
    const std::array<size_t, N> &full_pos) {
  if constexpr (D == 1) {
    co_yield full_pos;
  } else {
    std::array<size_t, D> starting_pos;
    std::copy(pos.begin(), pos.end(), starting_pos.begin());

    auto move_pos = [&]() -> bool {
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
      co_yield std::ranges::elements_of(
        diagonalScanCoreGen<D - 1, N>(
          size.template first<D - 1>(),
          pos.template first<D - 1>(),
          full_pos));
    } while (move_pos());

    std::copy(starting_pos.begin(), starting_pos.end(), pos.begin());
  }
}

template <size_t D>
std::generator<const std::array<size_t, D>&> diagonalScan(
    const std::array<size_t, D> &size, size_t diag) {
  std::array<size_t, D> pos {};
  for (size_t i = 0; i < D; i++) {
    size_t min = std::min(diag, size[i] - 1);
    pos[i] = min;
    diag -= min;
  }
  co_yield std::ranges::elements_of(
    diagonalScanCoreGen<D, D>(
      std::span<const size_t, D>{size},
      std::span<size_t, D>{pos},
      pos));
}
