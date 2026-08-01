#pragma once

#include "components/block.h"
#include "components/cabac.h"
#include "components/predict.h"

#include "prediction_type.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>


template <size_t D, typename T>
class BlockPredictor {
  DynamicBlock<T, D> decoded_image;

  void predict(DynamicBlock<T, D> &block, const std::array<size_t, D> &offset, PredictionType<D> type) {
    auto inputF = [&](std::array<int64_t, D> block_pos) -> T {
      if (offset == std::array<size_t, D>{}) {
        return 0.f;
      }

      // Prevent prediction from reading samples that have not been decoded yet.
      for (size_t i = 1; i <= D; i++) {
        size_t idx = D - i;

        if (block_pos[idx] < 0) {
          if (offset[idx] > 0) {
            break;
          }
        }
        else if (block_pos[idx] >= static_cast<int64_t>(block.size(idx))) {
          block_pos[idx] = block.size(idx) - 1;
        }
      }

      int64_t min_pos = std::numeric_limits<int64_t>::max();
      for (size_t i = 0; i < D; i++) {
        if (offset[i] > 0) {
          if (block_pos[i] < min_pos) {
            min_pos = block_pos[i];
          }
        }
        else if (block_pos[i] < 0) {
          block_pos[i] = 0;
        }
      }

      for (size_t i = 0; i < D; i++) {
        if (offset[i] > 0) {
          block_pos[i] -= min_pos + 1;
        }
      }

      std::array<size_t, D> image_pos {};
      for (size_t i = 0; i < D; i++) {
        image_pos[i] = std::clamp<size_t>(offset[i] + block_pos[i], 0, decoded_image.size(i) - 1);
      }

      return decoded_image[image_pos];
    };

    if (type.type == 0) {
      block.fill(0);
    }
    if (type.type == 1) {
      T value = predict_DC<D, T>(block.size(), inputF);
      for (size_t i = 0; i < block.stride(D); i++) {
        block[i] = value;
      }
    }
    else if (type.type == 2) {
      predict_planar<D>(block, inputF);
    }
    else if (type.type == 3) {
      predict_direction<D>(block, type.direction, inputF);
    }
  }

public:

  BlockPredictor(const std::array<size_t, D> &image_size): decoded_image(image_size) {};

  PredictionType<D> selectPredictionType(const DynamicBlock<T, D> &input_block, const std::array<size_t, D> &offset) {
    DynamicBlock<T, D> prediction_block(input_block.size());
    using Score = std::conditional_t<std::is_integral_v<T>, long double, T>;

    auto prediction_error = [&]() {
      Score error {};

      for (const auto &pos : iterate_dimensions<D>(input_block.size())) {
        const Score difference = static_cast<Score>(input_block[pos])
            - static_cast<Score>(prediction_block[pos]);
        error += difference * difference;
      }

      return error;
    };

    PredictionType<D> best_prediction_type {};
    Score lowest_error = prediction_error();

    auto eval_prediction = [&](PredictionType<D> type) {
      predict(prediction_block, offset, type);

      const Score current_error = prediction_error();
      if (current_error < lowest_error) {
        lowest_error = current_error;
        best_prediction_type = type;
      }
    };

    eval_prediction({1, {}});
    eval_prediction({2, {}});

    std::array<size_t, D> direction_range;
    direction_range.fill(5);
    for (const auto &pos : iterate_dimensions<D>(direction_range)) {
      std::array<int8_t, D> direction {};

      for (size_t i = 0; i < D; i++) {
        direction[i] = pos[i] - 2;
      }

      auto have_positive = [&]() {
        for (size_t d = 0; d < D; d++) {
          if (direction[d] > 0) {
            return true;
          }
        }
        return false;
      };

      auto have_eight = [&]() {
        for (size_t d = 0; d < D; d++) {
          if (std::abs(direction[d]) == 2) {
            return true;
          }
        }
        return false;
      };

      if (!have_positive() || !have_eight()) {
        continue;
      }

      eval_prediction({3, direction});
    }

    return best_prediction_type;
  }

  void forwardPass(DynamicBlock<T, D> &block, const std::array<size_t, D> &offset, PredictionType<D> type) {
    DynamicBlock<T, D> prediction_block(block.size());

    predict(prediction_block, offset, type);

    for (const auto &pos : iterate_dimensions<D>(block.size())) {
      if constexpr (std::is_integral_v<T>) {
        const int64_t value =
            static_cast<int64_t>(block[pos])
            - static_cast<int64_t>(prediction_block[pos]);
        if (value < std::numeric_limits<T>::min()
            || value > std::numeric_limits<T>::max()) {
          throw std::overflow_error("prediction residual overflow");
        }
        block[pos] = static_cast<T>(value);
      }
      else {
        block[pos] -= prediction_block[pos];
      }
    }
  }

  void backwardPass(DynamicBlock<T, D> &block, const std::array<size_t, D> &offset, PredictionType<D> type) {
    DynamicBlock<T, D> prediction_block(block.size());

    predict(prediction_block, offset, type);

    for (const auto &pos : iterate_dimensions<D>(block.size())) {
      if constexpr (std::is_integral_v<T>) {
        const int64_t value =
            static_cast<int64_t>(block[pos])
            + static_cast<int64_t>(prediction_block[pos]);
        if (value < std::numeric_limits<T>::min()
            || value > std::numeric_limits<T>::max()) {
          throw std::overflow_error("prediction reconstruction overflow");
        }
        block[pos] = static_cast<T>(value);
      }
      else {
        block[pos] += prediction_block[pos];
      }
    }

    moveBlock<D>([&](const auto &pos) { return block[pos]; }, block.size(), {},
                 [&](const auto &pos, const auto &val) { return decoded_image[pos] = val; }, decoded_image.size(), offset,
                 block.size());
  }
};
