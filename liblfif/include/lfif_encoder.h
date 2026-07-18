/**
* @file lfif_encoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for encoding an image.
*/

#pragma once

#include <cstdint>
#include <print>
#include <sstream>
#include <map>

#include "components/bitstream.h"
#include "components/colorspace.h"
#include "components/endian.h"

#include "block_predictor.h"
#include "dct_block_stream.h"
#include "dct_block_transformer.h"
#include "prediction_type_stream.h"
#include "lfif.h"

template <size_t D>
struct LFIFEncoder: public LFIF<D> {

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
    DynamicBlock<float, D> block_Y(this->block_size);
    DynamicBlock<float, D> block_U(this->block_size);
    DynamicBlock<float, D> block_V(this->block_size);

    DCTBlockTransformer<D> block_transformer(this->block_size, this->discarded_bits);

    DCTBlockStreamEncoder<D> block_encoder_Y(this->block_size);
    DCTBlockStreamEncoder<D> block_encoder_UV(this->block_size);

    std::array<size_t, D> predictor_size {};
    if (this->predicted) {
      predictor_size = this->size;
    }

    BlockPredictor<D, float> predictor_Y(predictor_size);
    BlockPredictor<D, float> predictor_U(predictor_size);
    BlockPredictor<D, float> predictor_V(predictor_size);

    PredictionTypeEncoder<D> prediction_type_encoder {};

    OBitstream   bitstream {};
    CABACEncoder cabac     {};

    bitstream.open(output);
    cabac.init(bitstream);

    std::array<size_t, D> aligned_image_size {};
    for (size_t i = 0; i < D; i++) {
      aligned_image_size[i] = (this->size[i] + this->block_size[i] - 1) / this->block_size[i] * this->block_size[i];
    }

    block_for<D>({}, this->block_size, aligned_image_size, [&](const std::array<size_t, D> &offset) {
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
                     block_Y[block_pos] = YCbCr::RGBToY(value[0], value[1], value[2]) - pow(2, this->depth_bits - 1);
                     block_U[block_pos] = YCbCr::RGBToCb(value[0], value[1], value[2]);
                     block_V[block_pos] = YCbCr::RGBToCr(value[0], value[1], value[2]);
                   }, this->block_size, {},
                   this->block_size);


      PredictionType<D> prediction_type {};
      if (this->predicted) {
        prediction_type = predictor_Y.selectPredictionType(block_Y, offset);

        predictor_Y.forwardPass(block_Y, offset, prediction_type);
        predictor_U.forwardPass(block_U, offset, prediction_type);
        predictor_V.forwardPass(block_V, offset, prediction_type);
      }

      block_transformer.forwardPass(block_Y);
      block_transformer.forwardPass(block_U);
      block_transformer.forwardPass(block_V);

      block_encoder_Y.encodeBlock(block_Y, cabac);
      block_encoder_UV.encodeBlock(block_U, cabac);
      block_encoder_UV.encodeBlock(block_V, cabac);

      if (this->predicted) {
        block_transformer.inversePass(block_Y);
        block_transformer.inversePass(block_U);
        block_transformer.inversePass(block_V);

        predictor_Y.backwardPass(block_Y, offset, prediction_type);
        predictor_U.backwardPass(block_U, offset, prediction_type);
        predictor_V.backwardPass(block_V, offset, prediction_type);

        prediction_type_encoder.encodePredictionType(prediction_type, cabac);
      }
    });

    cabac.terminate();
    bitstream.flush();
  }
};
