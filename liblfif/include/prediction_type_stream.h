#pragma once

#include "components/block.h"
#include "components/cabac.h"
#include "components/predict.h"

#include "prediction_type.h"

#include <cstddef>
#include <cstdint>

template <size_t D>
struct PredictionTypeStream {
  CABAC::ContextModel is_dc_prediction_ctx {};
  CABAC::ContextModel is_planar_prediction_ctx {};
  CABAC::ContextModel is_direction_prediction_ctx {};
  std::array<std::array<CABAC::ContextModel, 3>, D> directions_prediction_ctx {};
};

template <size_t D>
void encodePredictionType(
    PredictionTypeStream<D> &stream,
    const PredictionType<D> &type,
    auto &encoder) {
    encoder.encodeBit(stream.is_dc_prediction_ctx, type.type == 1);

    if (type.type != 1) {
      encoder.encodeBit(stream.is_planar_prediction_ctx, type.type == 2);

      if (type.type != 2) {
        encoder.encodeBit(stream.is_direction_prediction_ctx, type.type == 3);

        if (type.type == 3) {
          for (size_t i = 0; i < D; i++) {
            encoder.encodeBit(stream.directions_prediction_ctx[i][0], type.direction[i]);

            if (type.direction[i]) {
              encoder.encodeBit(stream.directions_prediction_ctx[i][1], std::abs(type.direction[i]) == 2);
              encoder.encodeBit(stream.directions_prediction_ctx[i][2], type.direction[i] < 0);
            }
          }
        }
      }
    }
}

template <size_t D>
PredictionType<D> decodePredictionType(
    PredictionTypeStream<D> &stream,
    auto &decoder) {
    PredictionType<D> type {};

    if (decoder.decodeBit(stream.is_dc_prediction_ctx)) {
      type.type = 1;
    }
    else if (decoder.decodeBit(stream.is_planar_prediction_ctx)) {
      type.type = 2;
    }
    else if (decoder.decodeBit(stream.is_direction_prediction_ctx)) {
      type.type = 3;

      for (size_t i = 0; i < D; i++) {
        type.direction[i] = decoder.decodeBit(stream.directions_prediction_ctx[i][0]);

        if (type.direction[i] != 0) {
          type.direction[i] += decoder.decodeBit(stream.directions_prediction_ctx[i][1]);

          if (decoder.decodeBit(stream.directions_prediction_ctx[i][2])) {
            type.direction[i] = -type.direction[i];
          }
        }
      }
    }

    return type;
}
