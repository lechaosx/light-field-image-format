#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <generator>
#include <ranges>
#include <span>

template<size_t Remaining, size_t D>
std::generator<const std::array<size_t, D> &> diagonalScanCore(
    std::span<const size_t, Remaining> size,
    std::span<size_t, Remaining> position,
    const std::array<size_t, D> &full_position) {
  if constexpr (Remaining == 1) {
    co_yield full_position;
  }
  else {
    std::array<size_t, Remaining> starting_position;
    std::ranges::copy(position, starting_position.begin());

    auto move = [&] {
      if (position[Remaining - 1] + 1 < size[Remaining - 1]) {
        for (size_t i = 2; i <= Remaining; ++i) {
          if (position[Remaining - i] > 0) {
            --position[Remaining - i];
            ++position[Remaining - 1];
            return true;
          }
        }
      }
      return false;
    };

    do {
      co_yield std::ranges::elements_of(
          diagonalScanCore<Remaining - 1>(
              size.template first<Remaining - 1>(),
              position.template first<Remaining - 1>(),
              full_position));
    } while (move());

    std::ranges::copy(starting_position, position.begin());
  }
}

template<size_t D>
std::generator<const std::array<size_t, D> &> diagonalScan(
    const std::array<size_t, D> &size,
    size_t diagonal) {
  std::array<size_t, D> position {};

  for (size_t dimension = 0; dimension < D; ++dimension) {
    position[dimension] = std::min(diagonal, size[dimension] - 1);
    diagonal -= position[dimension];
  }

  if (diagonal != 0) {
    co_return;
  }

  co_yield std::ranges::elements_of(
      diagonalScanCore<D>(
          std::span<const size_t, D> {size},
          std::span<size_t, D> {position},
          position));
}
