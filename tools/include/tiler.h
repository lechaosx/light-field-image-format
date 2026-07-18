/**
* @file tiler.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 5. 2. 2020
* @copyright 2019 Drahomír Dlabaja
*/

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

inline std::array<int64_t, 2> get_shift_coef(const std::array<size_t, 2> &pos, const std::array<size_t, 2> &size, const std::array<int64_t, 2> &shift_param) {
  return {(static_cast<int64_t>(pos[0]) - static_cast<int64_t>(size[0] / 2)) * shift_param[0], (static_cast<int64_t>(pos[1]) - static_cast<int64_t>(size[1] / 2)) * shift_param[1]};
}

inline std::array<size_t, 4> get_shifted_pos(std::array<size_t, 4> pos, const std::array<size_t, 4> &size, const std::array<int64_t, 2> &shift_param) {
  std::array<int64_t, 2> shifted {};

  shifted = get_shift_coef({pos[2], pos[3]}, {size[2], size[3]}, shift_param);

  for (size_t i {}; i < 2; i++) {
    shifted[i] = pos[i] + shifted[i];

    while (shifted[i] < 0) {
      shifted[i] += size[i];
    }

    pos[i] = shifted[i] % size[i];
  }

  return pos;
}

template <typename F>
std::array<int64_t, 2> find_best_shift_params(F &&inputF, const std::array<size_t, 4> &size, std::array<int64_t, 2> min, std::array<int64_t, 2> max, size_t iterations, int64_t sample_rate) {
  std::array<int64_t,  2> best_param {};
  std::array<uint64_t, 2> best_error {};

  std::array<std::map<int64_t, uint64_t>, 2> errors {};

  auto get_error_horizontal = [&](const std::array<int64_t, 2> &shift_param) {
    auto it = errors[0].find(shift_param[0]);
    if (it != errors[0].end()) {
      return it->second;
    }

    uint64_t error {};
    for (const auto &pos : iterate_dimensions<4>(std::array<size_t, 4> {size[0], size[1], size[2] - 1, size[3]})) {
      if (rand() < RAND_MAX / sample_rate) {
        std::array<uint16_t, 3> v0 = inputF(get_shifted_pos({pos[0], pos[1], pos[2] + 0, pos[3]}, size, shift_param));
        std::array<uint16_t, 3> v1 = inputF(get_shifted_pos({pos[0], pos[1], pos[2] + 1, pos[3]}, size, shift_param));

        error += std::abs(v0[0] - v1[0]);
        error += std::abs(v0[1] - v1[1]);
        error += std::abs(v0[2] - v1[2]);
      }
    }

    errors[0][shift_param[0]] = error;

    return error;
  };

  auto get_error_vertical = [&](const std::array<int64_t, 2> &shift_param) {
    auto it = errors[1].find(shift_param[1]);
    if (it != errors[1].end()) {
      return it->second;
    }

    uint64_t error {};
    for (const auto &pos : iterate_dimensions<4>(std::array<size_t, 4> {size[0], size[1], size[2], size[3] - 1})) {
      if (rand() < RAND_MAX / sample_rate) {
        std::array<uint16_t, 3> v0 = inputF(get_shifted_pos({pos[0], pos[1], pos[2], pos[3] + 0}, size, shift_param));
        std::array<uint16_t, 3> v1 = inputF(get_shifted_pos({pos[0], pos[1], pos[2], pos[3] + 1}, size, shift_param));

        error += std::abs(v0[0] - v1[0]);
        error += std::abs(v0[1] - v1[1]);
        error += std::abs(v0[2] - v1[2]);
      }
    }

    errors[1][shift_param[1]] = error;

    return error;
  };

  double golden_ratio = (sqrt(5.) + 1.) / 2.;

  best_error[0] = get_error_horizontal({0, 0});
  best_error[1] = get_error_vertical({0, 0});

  for (size_t i {}; i < iterations; i++) {
    std::array<int64_t,  2> param     {};
    std::array<uint64_t, 2> error_min {};
    std::array<uint64_t, 2> error_max {};

    for (size_t d {}; d < 2; d++) {
      param[d] = std::round(max[d] - (max[d] - min[d]) / golden_ratio);
    }

    error_min[0] = get_error_horizontal(param);
    error_min[1] = get_error_vertical(param);

    for (size_t d {}; d < 2; d++) {
      if (error_min[d] < best_error[d]) {
        best_error[d] = error_min[d];
        best_param[d] = param[d];
      }
    }

    for (size_t d {}; d < 2; d++) {
      param[d] = std::round(min[d] + (max[d] - min[d]) / golden_ratio);
    }

    error_max[0] = get_error_horizontal(param);
    error_max[1] = get_error_vertical(param);

    for (size_t d {}; d < 2; d++) {
      if (error_max[d] < best_error[d]) {
        best_error[d] = error_max[d];
        best_param[d] = param[d];
      }
    }

    for (size_t d {}; d < 2; d++) {
      if (error_min[d] < error_max[d]) {
        max[d] = std::round(min[d] + (max[d] - min[d]) / golden_ratio);
      }
      else {
        min[d] = std::round(max[d] - (max[d] - min[d]) / golden_ratio);
      }
    }

  }

  std::array<uint64_t, 2> error {};

  error[0] = get_error_horizontal(std::array {best_param[0], best_param[0]});
  error[1] = get_error_vertical(  std::array {best_param[0], best_param[0]});

  for (size_t d {}; d < 2; d++) {
    if (error[d] < best_error[d]) {
      best_error[d] = error[d];
      best_param[d] = best_param[0];
    }
  }

  error[0] = get_error_horizontal(std::array {best_param[1], best_param[1]});
  error[1] = get_error_vertical(  std::array {best_param[1], best_param[1]});

  for (size_t d {}; d < 2; d++) {
    if (error[d] < best_error[d]) {
      best_error[d] = error[d];
      best_param[d] = best_param[1];
    }
  }

  return best_param;
}
