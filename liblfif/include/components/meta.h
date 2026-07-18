/**
* @file meta.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Compile-time stuff.
*/

#pragma once

#include <cstddef>

#include <array>

template<class T>
inline constexpr T constpow(const T base, const unsigned exponent) {
    return (exponent == 0) ? 1 : (base * constpow(base, exponent - 1));
}

template<size_t D, typename T, typename F>
void iterate_dimensions(const T &range, F &&callback) {
  if constexpr (D == 0) {
    callback(std::array<size_t, 0>{});
  } else {
    for (size_t i = 0; i < range[D - 1]; i++) {
      iterate_dimensions<D - 1>(range, [&](const std::array<size_t, D - 1> &indices) {
        std::array<size_t, D> new_indices {};
        new_indices[D - 1] = i;
        for (size_t j = 0; j < D - 1; j++) {
          new_indices[j] = indices[j];
        }
        callback(new_indices);
      });
    }
  }
}

template<size_t D, typename T, typename F>
void block_for(const T &start, const T &step, const T &stop, F &&callback) {
  if constexpr (D == 0) {
    T pos {};
    callback(pos);
  } else {
    for (size_t i = start[D - 1]; i < stop[D - 1]; i += step[D - 1]) {
      block_for<D - 1>(start, step, stop, [&](T &indices) {
        indices[D - 1] = i;
        callback(indices);
      });
    }
  }
}

template<size_t BS, size_t D, typename F>
void iterate_cube(F &&callback) {
  if constexpr (D == 0) {
    callback(std::array<size_t, 0>{});
  } else {
    for (size_t i = 0; i < BS; i++) {
      iterate_cube<BS, D - 1>([&](const std::array<size_t, D - 1> &indices) {
        std::array<size_t, D> new_indices {};
        new_indices[D - 1] = i;
        for (size_t j = 0; j < D - 1; j++) {
          new_indices[j] = indices[j];
        }
        callback(new_indices);
      });
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

template<size_t D>
size_t get_stride(const size_t BS[D]) {
  if constexpr (D == 0) {
    return 1;
  } else {
    return get_stride<D - 1>(BS) * BS[D - 1];
  }
}

template<size_t D, size_t N>
size_t get_stride(const std::array<size_t, N> &size) {
  return get_stride<D>(size.data());
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

template <size_t D>
size_t num_diagonals(const std::array<size_t, D> &BS) {
  size_t diagonals_cnt {};

  for (size_t i {}; i < D; i++) {
    diagonals_cnt += BS[i] - 1;
  }

  return diagonals_cnt + 1;
}
