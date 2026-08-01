#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

template<size_t Remaining, size_t D, typename F>
void diagonalScanCore(
    const std::array<size_t, D> &size,
    std::array<size_t, D> &position,
    F &callback) {
  if constexpr (Remaining == 1) {
    callback(position);
  }
  else {
    const auto starting_position = position;

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
      diagonalScanCore<Remaining - 1>(size, position, callback);
    } while (move());

    position = starting_position;
  }
}

template<size_t D, typename F>
void diagonalScan(
    const std::array<size_t, D> &size,
    size_t diagonal,
    F &&callback) {
  std::array<size_t, D> position {};

  for (size_t i = 0; i < D; ++i) {
    const size_t coordinate = std::min(diagonal, size[i] - 1);
    position[i] = coordinate;
    diagonal -= coordinate;
  }

  diagonalScanCore<D>(size, position, callback);
}
