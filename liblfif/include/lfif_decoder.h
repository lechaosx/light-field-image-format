/**
* @file lfif_decoder.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for decoding an image.
*/

#pragma once

#include "components/bitstream.h"
#include "components/colorspace.h"

#include "block_predictor.h"
#include "dct_block_stream.h"
#include "dct_block_transformer.h"
#include "prediction_type_stream.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

template <size_t D>
struct LFIFDecoder {
  std::array<size_t, D> size;
  std::array<size_t, D> block_size;
  uint8_t depth_bits;
  uint8_t discarded_bits;
  bool predicted;

  template<typename F>
  void decodeStream(std::istream &input, F &&pusher) {
    DynamicBlock<float, D> block_Y(this->block_size);
    DynamicBlock<float, D> block_U(this->block_size);
    DynamicBlock<float, D> block_V(this->block_size);

    std::array<size_t, D> aligned_image_size {};
    for (size_t i = 0; i < D; i++) {
      const size_t blocks =
          this->size[i] / this->block_size[i]
          + (this->size[i] % this->block_size[i] != 0);
      aligned_image_size[i] = blocks * this->block_size[i];
    }

    IBitstream   bitstream {};
    CABACDecoder cabac     {};

    DCTBlockTransformer<D> block_transformer(this->block_size, this->discarded_bits);

    DCTBlockStreamDecoder<D> block_decoder_Y(this->block_size);
    DCTBlockStreamDecoder<D> block_decoder_UV(this->block_size);

    std::array<size_t, D> predictor_size {};
    if (this->predicted) {
      predictor_size = this->size;
    }

    BlockPredictor<D, float> predictor_Y(predictor_size);
    BlockPredictor<D, float> predictor_U(predictor_size);
    BlockPredictor<D, float> predictor_V(predictor_size);

    PredictionTypeDecoder<D> prediction_type_decoder {};

    bitstream.open(input);
    cabac.init(bitstream);

    block_for<D>({}, this->block_size, aligned_image_size, [&](const std::array<size_t, D> &offset) {
      block_decoder_Y.decodeBlock(cabac,  block_Y);
      block_decoder_UV.decodeBlock(cabac, block_U);
      block_decoder_UV.decodeBlock(cabac, block_V);

      block_transformer.inversePass(block_Y);
      block_transformer.inversePass(block_U);
      block_transformer.inversePass(block_V);

      if (this->predicted) {
        auto prediction_type = prediction_type_decoder.decodePredictionType(cabac);

        predictor_Y.backwardPass(block_Y, offset, prediction_type);
        predictor_U.backwardPass(block_U, offset, prediction_type);
        predictor_V.backwardPass(block_V, offset, prediction_type);
      }

      moveBlock<D>([&](const auto &pos) {
                     float Y  = block_Y[pos] + pow(2, this->depth_bits - 1);
                     float Cb = block_U[pos];
                     float Cr = block_V[pos];

                     const float R_value =
                         std::round(YCbCr::YCbCrToR(Y, Cb, Cr));
                     const float G_value =
                         std::round(YCbCr::YCbCrToG(Y, Cb, Cr));
                     const float B_value =
                         std::round(YCbCr::YCbCrToB(Y, Cb, Cr));
                     if (!std::isfinite(R_value)
                         || !std::isfinite(G_value)
                         || !std::isfinite(B_value)) {
                       throw std::runtime_error(
                           "decoded color component is not finite");
                     }
                     uint16_t R = std::clamp<float>(R_value, 0, std::pow(2, this->depth_bits) - 1);
                     uint16_t G = std::clamp<float>(G_value, 0, std::pow(2, this->depth_bits) - 1);
                     uint16_t B = std::clamp<float>(B_value, 0, std::pow(2, this->depth_bits) - 1);

                     return std::array<uint16_t, 3>({R, G, B});
                   }, this->block_size, {},
                   pusher, this->size, offset,
                   this->block_size);
    });

    cabac.terminate();
  }
};
