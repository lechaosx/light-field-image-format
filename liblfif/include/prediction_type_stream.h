#pragma once

#include <cstddef>
#include <cstdint>

#include "components/block.h"
#include "components/cabac.h"
#include "components/predict.h"

#include "prediction_type.h"

template <size_t D>
struct PredictionTypeStream {
  CABAC::ContextModel is_dc_prediction_ctx {};
  CABAC::ContextModel is_planar_prediction_ctx {};
  CABAC::ContextModel is_direction_prediction_ctx {};
  std::array<std::array<CABAC::ContextModel, 3>, D> directions_prediction_ctx {};
};

template <size_t D>
void encode_prediction_type(PredictionTypeStream<D> &s, const PredictionType<D> &type, CABACEncoder &encoder) {
  encoder.encodeBit(s.is_dc_prediction_ctx, type.type == 1);

  if (type.type != 1) {
    encoder.encodeBit(s.is_planar_prediction_ctx, type.type == 2);

    if (type.type != 2) {
      encoder.encodeBit(s.is_direction_prediction_ctx, type.type == 3);

      if (type.type == 3) {
        for (size_t i = 0; i < D; i++) {
          encoder.encodeBit(s.directions_prediction_ctx[i][0], type.direction[i]);

          if (type.direction[i]) {
            encoder.encodeBit(s.directions_prediction_ctx[i][1], std::abs(type.direction[i]) == 2);
            encoder.encodeBit(s.directions_prediction_ctx[i][2], type.direction[i] < 0);
          }
        }
      }
    }
  }
}

template <size_t D>
PredictionType<D> decode_prediction_type(PredictionTypeStream<D> &s, CABACDecoder &decoder) {
  PredictionType<D> type {};

  if (decoder.decodeBit(s.is_dc_prediction_ctx)) {
    type.type = 1;
  }
  else if (decoder.decodeBit(s.is_planar_prediction_ctx)) {
    type.type = 2;
  }
  else if (decoder.decodeBit(s.is_direction_prediction_ctx)) {
    type.type = 3;

    for (size_t i = 0; i < D; i++) {
      type.direction[i] = decoder.decodeBit(s.directions_prediction_ctx[i][0]);

      if (type.direction[i] != 0) {
        type.direction[i] += decoder.decodeBit(s.directions_prediction_ctx[i][1]);

        if (decoder.decodeBit(s.directions_prediction_ctx[i][2])) {
          type.direction[i] = -type.direction[i];
        }
      }
    }
  }

  return type;
}
