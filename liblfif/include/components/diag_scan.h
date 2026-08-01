#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <generator>

template<size_t D>
std::generator<std::array<size_t, D>> diagonalScan(
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

  while (true) {
    co_yield position;

    bool moved = false;
    for (size_t dimension = 1; dimension < D; ++dimension) {
      size_t lower_sum {};
      for (size_t lower = 0; lower < dimension; ++lower) {
        lower_sum += position[lower];
      }

      if (lower_sum == 0 || position[dimension] + 1 >= size[dimension]) {
        continue;
      }

      ++position[dimension];
      --lower_sum;
      for (size_t lower = 0; lower < dimension; ++lower) {
        position[lower] = std::min(lower_sum, size[lower] - 1);
        lower_sum -= position[lower];
      }
      moved = true;
      break;
    }

    if (!moved) {
      co_return;
    }
  }
}
