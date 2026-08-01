#pragma once

#include "block.h"
#include "meta.h"

#include <cstdint>
#include <cassert>
#include <cmath>
#include <span>
#include <type_traits>

template<size_t D, typename T, typename F>
void interpolate(
    std::span<const size_t, D> size,
    F &&main_ref,
    std::span<const int64_t, D> main_ref_pos,
    int8_t multiplier,
    T &output) {
  if constexpr (D == 0) {
    output = main_ref(0);
  }
  else {
    int64_t pos = std::floor(main_ref_pos[D - 1] / static_cast<double>(multiplier));
    int64_t frac = main_ref_pos[D - 1] % multiplier;

    auto inputF1 = [&](size_t index) {
      return main_ref(pos * get_stride<D - 1>(size.data()) + index);
    };

    if (frac == 0) {
      interpolate<D - 1>(
          size.template first<D - 1>(),
          inputF1,
          main_ref_pos.template first<D - 1>(),
          multiplier,
          output);
    }
    else {
      T val1 {};
      T val2 {};

      auto inputF2 = [&](size_t index) {
        return main_ref((pos + 1) * get_stride<D - 1>(size.data()) + index);
      };

      interpolate<D - 1>(
          size.template first<D - 1>(),
          inputF1,
          main_ref_pos.template first<D - 1>(),
          multiplier,
          val1);
      interpolate<D - 1>(
          size.template first<D - 1>(),
          inputF2,
          main_ref_pos.template first<D - 1>(),
          multiplier,
          val2);

      output = (val1 * (multiplier - frac) + val2 * frac) / multiplier;
    }
  }
}

template<size_t D, typename F>
void low_pass_sum(std::span<const size_t, D> size, F &&input) {
  if constexpr (D == 1) {
    auto prev_value = input(0);

    for (size_t i { 0 }; i < size[0] - 1; i++) {
      auto tmp = input(i);
      input(i) = prev_value + 2 * input(i) + input(i + 1);
      prev_value = tmp;
    }

    input(size[0] - 1) = prev_value + 3 * input(size[0] - 1);
  }
  else {
    for (size_t slice = 0; slice < size[D - 1]; slice++) {
      auto inputF = [&](size_t index) -> auto & {
        return input(slice * get_stride<D - 1>(size.data()) + index);
      };

      low_pass_sum<D - 1>(size.template first<D - 1>(), inputF);
    }

    for (size_t noodle = 0; noodle < get_stride<D - 1>(size.data()); noodle++) {
      auto inputF = [&](size_t index) -> auto & {
        return input(index * get_stride<D - 1>(size.data()) + noodle);
      };

      low_pass_sum<1>(size.template last<1>(), inputF);
    }
  }
}

template <size_t D, typename T>
void low_pass_filter(DynamicBlock<T, D> &main_ref) {
  if constexpr (std::is_integral_v<T>) {
    DynamicBlock<int64_t, D> values(main_ref.size());
    for (size_t i {}; i < main_ref.stride(D); ++i) {
      values[i] = main_ref[i];
    }
    auto inputF = [&](size_t index) -> auto & {
      return values[index];
    };
    low_pass_sum<D>(values.size(), inputF);
    for (size_t i {}; i < values.stride(D); ++i) {
      main_ref[i] = static_cast<T>(values[i] / constpow(4, D));
    }
  }
  else {
    auto inputF = [&](size_t index) -> auto & {
      return main_ref[index];
    };
    low_pass_sum<D>(main_ref.size(), inputF);
    for (size_t i {}; i < main_ref.stride(D); ++i) {
      main_ref[i] /= constpow(4, D);
    }
  }
}

template <size_t D, typename T, typename F>
void project_neighbours_to_main_ref(const std::array<size_t, D> &BS, DynamicBlock<T, D - 1> &main_ref, const int8_t direction[D], size_t main_ref_idx, F &&inputF) {
  std::array<int64_t, D> start_offsets {};
  std::array<int64_t, D> end_offsets   {};

  if (direction[main_ref_idx] < 0) {
    start_offsets[main_ref_idx] = BS[main_ref_idx];
  }

  for (size_t i = 0; i < D - 1; i++) {
    size_t idx = i < main_ref_idx ? i : i + 1;

    if (direction[idx] >= 0) {
      start_offsets[idx] = -BS[main_ref_idx];
    }
  }

  for (size_t i = 0; i < D; i++) {
    if (direction[i] > 0) {
      end_offsets[i] -= 1;
    }
  }

  for (const auto &pos : iterate_dimensions<D - 1>(main_ref.size())) {
    std::array<int64_t, D> position {};

    position[main_ref_idx] = start_offsets[main_ref_idx];
    for (size_t i {}; i < D - 1; i++) {
      size_t idx = i < main_ref_idx ? i : i + 1;
      position[idx] = pos[i] + start_offsets[idx];
    }

    double nearest_neighbour_distance = std::numeric_limits<double>::max();
    size_t nearest_neighbour_idx {};
    for (size_t i { 0 }; i < D; i++) {
      if (direction[i] > 0) {
        double distance = static_cast<double>(position[i]) / direction[i];

        if (distance <= nearest_neighbour_distance) {
          nearest_neighbour_distance = distance;
          nearest_neighbour_idx = i;
        }
      }
    }

    int64_t num_steps = position[nearest_neighbour_idx];
    for (size_t i { 0 }; i < D; i++) {
      position[i] *= direction[nearest_neighbour_idx];
      position[i] -= direction[i] * num_steps;
      position[i] /= direction[nearest_neighbour_idx];

      if (position[i] < 0) {
        position[i] = 0;
      }
      else if ((direction[i] == 0) && (position[i] >= static_cast<int64_t>(BS[i]))) {
        position[i] = BS[i] - 1;
      }
      else if ((direction[i] > 0) && (position[i] > static_cast<int64_t>(BS[i]))) {
        position[i] = BS[i];
      }

      position[i] += end_offsets[i];
    }

    main_ref[pos] = inputF(position);
  }
}

template <size_t D, typename T>
void predict_from_main_ref(DynamicBlock<T, D> &output, const int8_t direction[D], const DynamicBlock<T, D - 1> &main_ref, size_t main_ref_idx) {
  std::array<int64_t, D> offsets {};

  for (size_t i = 0; i < D; i++) {
    if (direction[i] > 0) {
      offsets[i] += 1;
    }

    if (direction[i] >= 0) {
      offsets[i] += output.size(main_ref_idx);
    }
  }

  // The main-reference offset must override the generic dimensional offsets.
  if (direction[main_ref_idx] < 0) {
    offsets[main_ref_idx] = -output.size(main_ref_idx);
  }
  else {
    offsets[main_ref_idx] = 1;
  }

  for (const auto &pos : iterate_dimensions<D>(output.size())) {
    int64_t distance = pos[main_ref_idx] + offsets[main_ref_idx];

    int64_t main_ref_pos[D - 1] {};
    for (size_t i { 0 }; i < D - 1; i++) {
      size_t idx = i < main_ref_idx ? i : i + 1;
      main_ref_pos[i] = pos[idx] + offsets[idx];

      main_ref_pos[i] *= direction[main_ref_idx];
      main_ref_pos[i] -= direction[idx] * distance;
    }

    auto inputF = [&](size_t index) {
      return main_ref[index];
    };

    interpolate<D - 1>(
        main_ref.size(),
        inputF,
        main_ref_pos,
        direction[main_ref_idx],
        output[pos]);
  }
}

template <size_t D, typename T, typename F>
void predict_direction(DynamicBlock<T, D> &output, const int8_t direction[D], F &&inputF) {
  size_t  main_ref_idx { 0 };

  auto positive = [&]() {
    for (size_t i = 0; i < D; i++) {
      if (direction[i] > 0) {
        return true;
      }
    }
    return false;
  };

  if (!positive()) {
    return;
  }

  for (size_t d = 0; d < D; d++) {
    if (std::abs(direction[d]) >= std::abs(direction[main_ref_idx])) {
      main_ref_idx = d;
    }
  }

  size_t ref_size[D - 1] {};

  for (size_t i {}; i < D - 1; i++) {
    size_t idx = i < main_ref_idx ? i : i + 1;
    ref_size[i] = output.size(idx) + output.size(main_ref_idx) + 1;
  }

  DynamicBlock<T, D - 1> ref(ref_size);

  project_neighbours_to_main_ref<D>(output.size(), ref, direction, main_ref_idx, inputF);
  low_pass_filter<D - 1>(ref);
  predict_from_main_ref<D>(output, direction, ref, main_ref_idx);
}

template<size_t D, typename  T, typename F>
T predict_DC(const std::array<size_t, D> &size, F &inputF) {
  using Sum = std::conditional_t<std::is_integral_v<T>, int64_t, T>;
  Sum    sum         {};
  size_t samples_cnt {};

  for (size_t neighbour_idx = 0; neighbour_idx < D; neighbour_idx++) {
    std::array<size_t, D - 1> neighbour_block_size {};

    for (size_t i = 0; i < D - 1; i++) {
      size_t idx              = i < neighbour_idx ? i : i + 1;
      neighbour_block_size[i] = size[idx];
    }

    samples_cnt += get_stride<D - 1>(neighbour_block_size.data());

    for (const auto &pos :
         iterate_dimensions<D - 1>(neighbour_block_size)) {
      std::array<int64_t, D> position {};

      for (size_t i { 0 }; i < D - 1; i++) {
        size_t idx     = i < neighbour_idx ? i : i + 1;
        position[idx] = pos[i];
      }

      position[neighbour_idx]--;

      sum += static_cast<Sum>(inputF(position));
    }
  }

  return static_cast<T>(sum / static_cast<Sum>(samples_cnt));
}

template<size_t D, typename T, typename F>
void predict_planar(DynamicBlock<T, D> &output, F &inputF) {
  using Sum = std::conditional_t<std::is_integral_v<T>, int64_t, T>;
  DynamicBlock<Sum, D> sum(output.size());

  for (size_t neighbour_idx { 0 }; neighbour_idx < D; neighbour_idx++) {
    DynamicBlock<T, D> tmp_prediction(output.size());

    int8_t direction[D] {};
    direction[neighbour_idx] = 1;

    predict_direction<D>(tmp_prediction, direction, inputF);

    for (size_t i { 0 }; i < get_stride<D>(tmp_prediction.size()); i++) {
      sum[i] += static_cast<Sum>(tmp_prediction[i]);
    }
  }

  for (size_t i { 0 }; i < output.stride(D); i++) {
    output[i] = static_cast<T>(sum[i] / static_cast<Sum>(D));
  }
}
