#pragma once

#include <array>
#include <cstddef>
#include <generator>

template<class T>
inline constexpr T constpow(const T base, const unsigned exponent) {
  return (exponent == 0) ? 1 : (base * constpow(base, exponent - 1));
}

template<size_t D, typename T>
std::generator<std::array<size_t, D>> iterate_dimensions(T range) {
  std::array<size_t, D> position {};

  if constexpr (D == 0) {
    co_yield position;
    co_return;
  }

  for (size_t dimension {}; dimension < D; ++dimension) {
    if (range[dimension] == 0) {
      co_return;
    }
  }

  while (true) {
    co_yield position;

    for (size_t dimension {}; dimension < D; ++dimension) {
      if (++position[dimension] < range[dimension]) {
        break;
      }
      position[dimension] = 0;
      if (dimension == D - 1) {
        co_return;
      }
    }
  }
}

template<size_t D>
std::generator<std::array<size_t, D>> block_for(
    std::array<size_t, D> start,
    std::array<size_t, D> step,
    std::array<size_t, D> stop) {
  for (size_t dimension {}; dimension < D; ++dimension) {
    if (start[dimension] >= stop[dimension]) {
      co_return;
    }
  }

  auto position = start;
  while (true) {
    co_yield position;

    for (size_t dimension {}; dimension < D; ++dimension) {
      position[dimension] += step[dimension];
      if (position[dimension] < stop[dimension]) {
        break;
      }
      position[dimension] = start[dimension];
      if (dimension == D - 1) {
        co_return;
      }
    }
  }
}

template<size_t BS, size_t D>
size_t make_cube_index(const std::array<size_t, D> &pos) {
  size_t index {};

  for (size_t i { 1 }; i <= D; i++) {
    index *= BS;
    index += pos[D - i];
  }

  return index;
}

template<size_t D, typename T>
size_t get_stride(const T &size) {
  size_t stride {1};
  for (size_t dimension {}; dimension < D; ++dimension) {
    stride *= size[dimension];
  }
  return stride;
}

template<size_t D, typename T>
size_t make_index(const T &BS, const std::array<size_t, D> &pos) {
  size_t index {};

  for (size_t i { 1 }; i <= D; i++) {
    index *= BS[D - i];
    index += pos[D - i];
  }

  return index;
}

template <size_t D, typename T>
size_t num_diagonals(const T &BS) {
  size_t diagonals_cnt {};

  for (size_t i {}; i < D; i++) {
    diagonals_cnt += BS[i] - 1;
  }

  return diagonals_cnt + 1;
}
