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

/**
* @brief  Function which performs integer exponentiation.
* @param  base Base of exponentiation.
* @param  exponent Exponent.
* @return base to exponent
*/
template<class T>
inline constexpr T constpow(const T base, const unsigned exponent) {
    return (exponent == 0) ? 1 : (base * constpow(base, exponent - 1));
}

template<size_t D>
struct iterate_dimensions {
  template<typename F, typename T>
  iterate_dimensions(const T &range, F &&callback) {
    for (size_t i { 0 }; i < range[D - 1]; i++) {
      auto new_callback = [&](const std::array<size_t, D - 1> &indices) {
        std::array<size_t, D> new_indices {};

        new_indices[D - 1] = i;
        for (size_t j { 0 }; j < D - 1; j++) {
          new_indices[j] = indices[j];
        }

        callback(new_indices);
      };

      iterate_dimensions<D - 1>(range, new_callback);
    }
  }
};

template<>
struct iterate_dimensions<0> {
  template<typename F, typename T>
  iterate_dimensions(const T &, F &&callback) {
    callback({});
  }
};

template<size_t D>
struct block_for {
  template<typename F, typename T>
  block_for(const T &start, const T &step, const T &stop, F &&callback) {
    for (size_t i = start[D - 1]; i < stop[D - 1]; i += step[D - 1]) {
      auto new_callback = [&](T &indices) {
        indices[D - 1] = i;
        callback(indices);
      };

      block_for<D - 1>(start, step, stop, new_callback);
    }
  }
};

template<>
struct block_for<0> {
  template<typename F, typename T>
  block_for(const T &, const T &, const T &, F &&callback) {
    T pos {};
    callback(pos);
  }
};

template<size_t BS, size_t D>
struct iterate_cube {
  template<typename F>
  iterate_cube(F &&callback) {
    for (size_t i { 0 }; i < BS; i++) {
      auto new_callback = [&](const std::array<size_t, D - 1> &indices) {
        std::array<size_t, D> new_indices {};

        new_indices[D - 1] = i;
        for (size_t j { 0 }; j < D - 1; j++) {
          new_indices[j] = indices[j];
        }

        callback(new_indices);
      };

      iterate_cube<BS, D - 1>{new_callback};
    }
  }
};

template<size_t BS>
struct iterate_cube<BS, 0> {
  template<typename F>
  iterate_cube(F &&callback) {
    callback({});
  }
};

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
  return get_stride<D - 1>(BS) * BS[D - 1];
}

template<>
inline size_t get_stride<0>(const size_t *) {
  return 1;
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

template <size_t D, typename T>
size_t num_diagonals(const T &BS) {
  size_t diagonals_cnt {};

  for (size_t i {}; i < D; i++) {
    diagonals_cnt += BS[i] - 1;
  }

  return diagonals_cnt + 1;
}
