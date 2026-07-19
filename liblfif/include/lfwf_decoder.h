/**
* @file lfwf_decoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for decoding an image.
*/

#pragma once

#include <cstdint>
#include <print>

#include "components/bitstream.h"
#include "components/colorspace.h"
#include "components/endian.h"

#include "block_predictor.h"
#include "dwt_block_transformer.h"
#include "dwt_block_stream.h"
#include "prediction_type_stream.h"

template <size_t D>
struct LFWFDecoder {
  std::array<size_t, D> size;
  std::array<size_t, D> block_size;
  uint8_t depth_bits;
  uint8_t discarded_bits;
  bool predicted;

  void open(std::istream &input) {
    this->depth_bits     = readValueFromStream<uint8_t>(input);
    this->discarded_bits = readValueFromStream<uint8_t>(input);
    this->predicted      = readValueFromStream<bool>(input);

    for (size_t i = 0; i < D; i++) {
      this->size[i] = readValueFromStream<uint64_t>(input);
    }

    for (size_t i = 0; i < D; i++) {
      this->block_size[i] = readValueFromStream<uint64_t>(input);
    }
  }

  template<typename F>
  void decodeStream(std::istream &input, F &&pusher) {
    DynamicBlock<int32_t, D> block_Y(this->block_size);
    DynamicBlock<int32_t, D> block_U(this->block_size);
    DynamicBlock<int32_t, D> block_V(this->block_size);

    std::array<size_t, D> aligned_image_size {};
    for (size_t i = 0; i < D; i++) {
      aligned_image_size[i] = (this->size[i] + this->block_size[i] - 1) / this->block_size[i] * this->block_size[i];
    }

    IBitstream   bitstream {};
    CABACDecoder cabac     {};

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

    ::open(bitstream, input);
    cabac.init(bitstream);

    for (const auto &offset : block_for<D>({}, this->block_size, aligned_image_size)) {
      for (size_t i = 0; i < D; i++) {
        std::print(stderr, "{} ", offset[i]);
      }
      std::print(stderr, "out of ");
      for (size_t i = 0; i < D; i++) {
        std::print(stderr, "{} ", aligned_image_size[i]);
      }
      std::println(stderr);

      decode_dwt_block(stream_Y,  block_Y, cabac);
      decode_dwt_block(stream_UV, block_U, cabac);
      decode_dwt_block(stream_UV, block_V, cabac);

      dwt_inverse(block_Y, this->discarded_bits);
      dwt_inverse(block_U, this->discarded_bits);
      dwt_inverse(block_V, this->discarded_bits);

      if (this->predicted) {
        auto prediction_type = decode_prediction_type(pred_type_stream, cabac);

        predictor_Y.backwardPass(block_Y, offset, prediction_type);
        predictor_U.backwardPass(block_U, offset, prediction_type);
        predictor_V.backwardPass(block_V, offset, prediction_type);
      }

      moveBlock<D>([&](const auto &pos) {
                     int32_t Y  = block_Y[pos] + (1 << (this->depth_bits - 1));
                     int32_t U = block_U[pos];
                     int32_t V = block_V[pos];

                     uint16_t G = std::clamp<int32_t>(Y - ((U + V) >> 2), 0, (1 << this->depth_bits) - 1);
                     uint16_t R = std::clamp<int32_t>(V + G,              0, (1 << this->depth_bits) - 1);
                     uint16_t B = std::clamp<int32_t>(U + G,              0, (1 << this->depth_bits) - 1);

                     return std::array<uint16_t, 3>({R, G, B});
                   }, this->block_size, {},
                   pusher, this->size, offset,
                   this->block_size);
    }

    cabac.terminate();
  }
};
