#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <generator>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>

template<class T>
inline constexpr T constpow(const T base, const unsigned exponent) {
  return (exponent == 0) ? 1 : (base * constpow(base, exponent - 1));
}

template<typename F, typename T>
concept LinearReference = requires(F &&function, size_t index) {
  { std::forward<F>(function)(index) } -> std::same_as<T &>;
};

template<size_t Remaining, size_t D>
std::generator<const std::array<size_t, D> &> iterate_dimensions_core(
    const std::array<size_t, D> &range,
    std::array<size_t, D> &position) {
  if constexpr (Remaining == 0) {
    co_yield position;
  }
  else {
    for (position[Remaining - 1] = 0;
         position[Remaining - 1] < range[Remaining - 1];
         ++position[Remaining - 1]) {
      co_yield std::ranges::elements_of(
          iterate_dimensions_core<Remaining - 1>(range, position));
    }
  }
}

template<size_t D>
std::generator<std::array<size_t, D>> iterate_dimensions(
    std::array<size_t, D> range) {
  std::array<size_t, D> position {};
  for (const auto &current : iterate_dimensions_core<D>(range, position)) {
    co_yield current;
  }
}

template<size_t Remaining, size_t D>
std::generator<const std::array<size_t, D> &> block_for_core(
    const std::array<size_t, D> &start,
    const std::array<size_t, D> &step,
    const std::array<size_t, D> &stop,
    std::array<size_t, D> &position) {
  if constexpr (Remaining == 0) {
    co_yield position;
  }
  else {
    for (position[Remaining - 1] = start[Remaining - 1];
         position[Remaining - 1] < stop[Remaining - 1];
         position[Remaining - 1] += step[Remaining - 1]) {
      co_yield std::ranges::elements_of(
          block_for_core<Remaining - 1>(start, step, stop, position));
    }
  }
}

template<size_t D>
std::generator<std::array<size_t, D>> block_for(
    std::array<size_t, D> start,
    std::array<size_t, D> step,
    std::array<size_t, D> stop) {
  std::array<size_t, D> position {};
  for (const auto &current : block_for_core<D>(start, step, stop, position)) {
    co_yield current;
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

template<size_t D, size_t N>
requires (D <= N)
size_t get_stride(const std::array<size_t, N> &size) {
  return std::reduce(
      size.begin(), size.begin() + D, size_t {1}, std::multiplies {});
}

template<size_t D, size_t N>
requires (D <= N)
size_t get_stride(std::span<const size_t, N> size) {
  return std::reduce(
      size.begin(), size.begin() + D, size_t {1}, std::multiplies {});
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
