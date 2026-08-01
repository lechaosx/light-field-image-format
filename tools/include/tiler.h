#pragma once

#include <components/block.h>
#include <components/meta.h>

#include <cmath>
#include <numeric>

#include <array>
#include <map>

template <typename IF, typename OF>
inline void shift_image(IF &&input, OF &&output, const std::array<size_t, 2> &dimensions, std::array<int64_t, 2> shift_coef) {
  std::array<size_t, 2> gcd {};

  for (size_t i {}; i < 2; i++) {
    while (shift_coef[i] < 0) {
      shift_coef[i] += dimensions[i];
    }
    shift_coef[i] %= dimensions[i];

    gcd[i] = std::gcd<size_t>(dimensions[i], shift_coef[i]);
  }

  for (size_t y {}; y < dimensions[1]; y++) {
    for (size_t x {}; x < gcd[0]; x++) {
      auto tmp = input({x, y});

      size_t i {x};
      while (true) {
        size_t k = i + shift_coef[0];
        if (k >= dimensions[0]) {
          k -= dimensions[0];
        }

        if (k == x) {
          break;
        }

        output({i, y}, input({k, y}));
        i = k;
      }

      output({i, y}, tmp);
    }
  }

  for (size_t y {}; y < gcd[1]; y++) {
    for (size_t x {}; x < dimensions[0]; x++) {
      auto tmp = input({x, y});

      size_t i {y};
      while (true) {
        size_t k = i + shift_coef[1];
        if (k >= dimensions[1]) {
          k -= dimensions[1];
        }

        if (k == y) {
          break;
        }

        output({x, i}, input({x, k}));
        i = k;
      }

      output({x, i}, tmp);
    }
  }
}

inline std::array<size_t, 4> get_shifted_pos(std::array<size_t, 4> pos, const std::array<size_t, 4> &size, const std::array<int64_t, 2> &shift_param) {
  for (size_t i {}; i < 2; i++) {
    int64_t shifted =
        static_cast<int64_t>(pos[i])
        + (static_cast<int64_t>(pos[i + 2])
           - static_cast<int64_t>(size[i + 2] / 2))
              * shift_param[i];
    shifted %= static_cast<int64_t>(size[i]);

    if (shifted < 0) {
      shifted += size[i];
    }

    pos[i] = static_cast<size_t>(shifted);
  }

  return pos;
}

template <typename F>
std::array<int64_t, 2> find_best_shift_params(
    F &&input,
    const std::array<size_t, 4> &size) {
  std::array<int64_t, 2> best_shift {};

  for (size_t axis {}; axis < 2; ++axis) {
    if (size[axis + 2] < 2) {
      continue;
    }

    auto error = [&](int64_t shift) {
      std::array<int64_t, 2> shifts {};
      shifts[axis] = shift;
      auto range = size;
      --range[axis + 2];
      uint64_t total {};

      for (const auto &position : iterate_dimensions<4>(range)) {
        auto adjacent = position;
        ++adjacent[axis + 2];
        const auto first =
            input(get_shifted_pos(position, size, shifts));
        const auto second =
            input(get_shifted_pos(adjacent, size, shifts));
        for (size_t channel {}; channel < first.size(); ++channel) {
          total += static_cast<uint64_t>(std::abs(
              static_cast<int>(first[channel])
              - static_cast<int>(second[channel])));
        }
      }

      return total;
    };

    uint64_t best_error = error(0);
    const int64_t limit = static_cast<int64_t>(
        std::min<size_t>(size[axis] / 2, 32));
    for (int64_t magnitude = 1; magnitude <= limit; ++magnitude) {
      for (const int64_t candidate : {-magnitude, magnitude}) {
        const uint64_t candidate_error = error(candidate);
        if (candidate_error < best_error) {
          best_error = candidate_error;
          best_shift[axis] = candidate;
        }
      }
    }
  }

  return best_shift;
}
