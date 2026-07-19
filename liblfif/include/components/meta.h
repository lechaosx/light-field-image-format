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
#include <generator>
#include <numeric>
#include <ranges>
#include <span>

template<class T>
inline constexpr T constpow(const T base, const unsigned exponent) {
    return (exponent == 0) ? 1 : (base * constpow(base, exponent - 1));
}

template <typename F, typename T>
concept LinearRef = requires(F f, size_t i) {
  { f(i) } -> std::same_as<T&>;
};

template<size_t D, typename T>
auto iterate_dimensions(T range) {
  return [range]<size_t... I>(std::index_sequence<I...>)
      -> std::generator<const std::array<size_t, D>&> {
    for (const auto &tuple : std::views::cartesian_product(
        std::views::iota(size_t{0}, range[D - 1 - I])...)) {
      auto [...elems] = tuple;
      co_yield std::array<size_t, D>{ static_cast<size_t>(elems...[D - 1 - I])... };
    }
  }(std::make_index_sequence<D>{});
}

template<size_t D, typename T>
auto block_for(T start, T step, T stop) {
  return [start, step, stop]<size_t... I>(std::index_sequence<I...>)
      -> std::generator<const std::array<size_t, D>&> {
    for (const auto &tuple : std::views::cartesian_product(
        (std::views::iota(start[D - 1 - I], stop[D - 1 - I])
         | std::views::stride(step[D - 1 - I]))...)) {
      auto [...elems] = tuple;
      co_yield std::array<size_t, D>{ static_cast<size_t>(elems...[D - 1 - I])... };
    }
  }(std::make_index_sequence<D>{});
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
