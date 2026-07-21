#pragma once

#include <cstddef>

#include <array>
#include <concepts>
#include <functional>
#include <generator>
#include <numeric>
#include <span>

template<class T>
inline constexpr T constpow(const T base, const unsigned exponent) {
    return (exponent == 0) ? 1 : (base * constpow(base, exponent - 1));
}

template <typename F, typename T>
concept LinearRef = requires(F f, size_t i) {
  { f(i) } -> std::same_as<T&>;
};

template<size_t D, size_t N>
std::generator<const std::array<size_t, N>&> iterate_dimensions_core(
    const std::array<size_t, N> &range, std::array<size_t, N> &pos) {
  if constexpr (D == 0) {
    co_yield pos;
  } else {
    for (pos[D - 1] = 0; pos[D - 1] < range[D - 1]; pos[D - 1]++) {
      co_yield std::ranges::elements_of(iterate_dimensions_core<D - 1, N>(range, pos));
    }
  }
}

template<size_t D, typename T>
std::generator<const std::array<size_t, D>&> iterate_dimensions(T range) {
  std::array<size_t, D> pos {};
  co_yield std::ranges::elements_of(iterate_dimensions_core<D, D>(range, pos));
}

template<size_t D, size_t N>
std::generator<const std::array<size_t, N>&> block_for_core(
    const std::array<size_t, N> &start,
    const std::array<size_t, N> &step,
    const std::array<size_t, N> &stop,
    std::array<size_t, N> &pos) {
  if constexpr (D == 0) {
    co_yield pos;
  } else {
    for (pos[D - 1] = start[D - 1]; pos[D - 1] < stop[D - 1]; pos[D - 1] += step[D - 1]) {
      co_yield std::ranges::elements_of(block_for_core<D - 1, N>(start, step, stop, pos));
    }
  }
}

template<size_t D, typename T>
std::generator<const std::array<size_t, D>&> block_for(T start, T step, T stop) {
  std::array<size_t, D> pos {};
  co_yield std::ranges::elements_of(block_for_core<D, D>(start, step, stop, pos));
}

template<size_t BS, size_t D>
std::generator<const std::array<size_t, D>&> iterate_cube() {
  std::array<size_t, D> sizes {};
  sizes.fill(BS);
  co_yield std::ranges::elements_of(iterate_dimensions<D>(sizes));
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

template<size_t D, size_t N>
size_t get_stride(const std::array<size_t, N> &size) {
  return std::reduce(size.begin(), size.begin() + D, size_t{1}, std::multiplies{});
}

template<size_t D, size_t N>
size_t get_stride(std::span<const size_t, N> size) {
  return std::reduce(size.begin(), size.begin() + D, size_t{1}, std::multiplies{});
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
