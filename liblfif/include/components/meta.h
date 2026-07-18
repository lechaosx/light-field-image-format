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
#include <concepts>
#include <functional>
#include <numeric>
#include <ranges>
#include <span>

template<class T>
inline constexpr T constpow(const T base, const unsigned exponent) {
    return (exponent == 0) ? 1 : (base * constpow(base, exponent - 1));
}

template <typename F, size_t D>
concept DimCallback = std::invocable<F, const std::array<size_t, D>&>;

template <typename F, typename T>
concept LinearRef = requires(F f, size_t i) {
  { f(i) } -> std::same_as<T&>;
};

template<size_t D, typename T, DimCallback<D> F>
void iterate_dimensions(const T &range, F &&callback) {
  [&]<size_t... I>(std::index_sequence<I...>) {
    for (const auto &tuple : std::views::cartesian_product(
        std::views::iota(size_t{0}, range[D - 1 - I])...)) {
      callback(std::array<size_t, D>{ std::get<D - 1 - I>(tuple)... });
    }
  }(std::make_index_sequence<D>{});
}

template<size_t D, typename T, typename F>
void block_for(const T &start, const T &step, const T &stop, F &&callback) {
  [&]<size_t... I>(std::index_sequence<I...>) {
    for (const auto &tuple : std::views::cartesian_product(
        (std::views::iota(start[D - 1 - I], stop[D - 1 - I])
         | std::views::stride(step[D - 1 - I]))...)) {
      T pos { static_cast<size_t>(std::get<D - 1 - I>(tuple))... };
      callback(pos);
    }
  }(std::make_index_sequence<D>{});
}

template<size_t BS, size_t D, DimCallback<D> F>
void iterate_cube(F &&callback) {
  std::array<size_t, D> sizes {};
  sizes.fill(BS);
  iterate_dimensions<D>(sizes, std::forward<F>(callback));
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
