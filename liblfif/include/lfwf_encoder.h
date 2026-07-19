/**
* @file lfwf_encoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 28. 1. 2021
* @copyright 2021 Drahomír Dlabaja
* @brief Class for encoding an image with wavelets.
*/

#pragma once

#include <cstdint>
#include <print>

#include "components/bitstream.h"
#include "components/endian.h"

#include "block_predictor.h"
#include "dwt_block_stream.h"
#include "dwt_block_transformer.h"
#include "prediction_type_stream.h"

template <size_t D>
struct LFWFEncoder {
  std::array<size_t, D> size;
  std::array<size_t, D> block_size;
  uint8_t depth_bits;
  uint8_t discarded_bits;
  bool predicted;

  void create(
            std::ostream          &output,
      const std::array<size_t, D> &size,
      const std::array<size_t, D> &block_size,
            uint8_t                depth_bits,
            uint8_t                discarded_bits,
            bool                   predicted) {

    this->size = size;
    this->block_size = block_size;
    this->depth_bits = depth_bits;
    this->discarded_bits = discarded_bits;
    this->predicted = predicted;

    writeValueToStream<uint8_t>(depth_bits, output);
    writeValueToStream<uint8_t>(discarded_bits, output);
    writeValueToStream<bool>(predicted, output);

    for (size_t i = 0; i < D; i++) {
      writeValueToStream<uint64_t>(size[i], output);
    }

    for (size_t i = 0; i < D; i++) {
      writeValueToStream<uint64_t>(block_size[i], output);
    }
  }

  template <typename F>
  void encodeStream(F &&puller, std::ostream &output) {
    DynamicBlock<int32_t, D> block_Y(this->block_size);
    DynamicBlock<int32_t, D> block_U(this->block_size);
    DynamicBlock<int32_t, D> block_V(this->block_size);

    DWTBlockStream<D> stream_Y {};
    DWTBlockStream<D> stream_UV {};

    std::array<size_t, D> predictor_size {};
    if (this->predicted) {
      predictor_size = this->size;
    }

    BlockPredictor<D, int32_t> predictor_Y(predictor_size);
    BlockPredictor<D, int32_t> predictor_U(predictor_size);
    BlockPredictor<D, int32_t> predictor_V(predictor_size);

    PredictionTypeStream<D> pred_type_stream {};

    OBitstream   bitstream {};
    CABACEncoder cabac     {};

    bitstream.open(output);
    cabac.init(bitstream);

    std::array<size_t, D> aligned_image_size {};
    for (size_t i = 0; i < D; i++) {
      aligned_image_size[i] = (this->size[i] + this->block_size[i] - 1) / this->block_size[i] * this->block_size[i];
    }

    for (const auto &offset : block_for<D>({}, this->block_size, aligned_image_size)) {
      for (size_t i = 0; i < D; i++) {
        std::print(stderr, "{} ", offset[i]);
      }
      std::print(stderr, "out of ");
      for (size_t i = 0; i < D; i++) {
        std::print(stderr, "{} ", aligned_image_size[i]);
      }
      std::println(stderr);

      moveBlock<D>(puller, this->size, offset,
                   [&](const auto &block_pos, const auto &value) {
                     block_Y[block_pos] = ((value[0] + (value[1] << 1) + value[2]) >> 2) - (1 << (this->depth_bits - 1));
                     block_U[block_pos] = value[2] - value[1];
                     block_V[block_pos] = value[0] - value[1];
                   }, this->block_size, {},
                   this->block_size);

      PredictionType<D> prediction_type {};
      if (this->predicted) {
        prediction_type = predictor_Y.selectPredictionType(block_Y, offset);

        predictor_Y.forwardPass(block_Y, offset, prediction_type);
        predictor_U.forwardPass(block_U, offset, prediction_type);
        predictor_V.forwardPass(block_V, offset, prediction_type);
      }

      dwt_forward(block_Y, this->discarded_bits);
      dwt_forward(block_U, this->discarded_bits);
      dwt_forward(block_V, this->discarded_bits);

      encode_dwt_block(stream_Y,  block_Y, cabac);
      encode_dwt_block(stream_UV, block_U, cabac);
      encode_dwt_block(stream_UV, block_V, cabac);

      if (this->predicted) {
        dwt_inverse(block_Y, this->discarded_bits);
        dwt_inverse(block_U, this->discarded_bits);
        dwt_inverse(block_V, this->discarded_bits);

        predictor_Y.backwardPass(block_Y, offset, prediction_type);
        predictor_U.backwardPass(block_U, offset, prediction_type);
        predictor_V.backwardPass(block_V, offset, prediction_type);

        encode_prediction_type(pred_type_stream, prediction_type, cabac);
      }
    }

    cabac.terminate();
    bitstream.flush();
  }
};
