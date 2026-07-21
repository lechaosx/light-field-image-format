#pragma once

#include <cstdint>
#include <print>

#include "components/bitstream.h"
#include "components/colorspace.h"
#include "components/lfif_header.h"

#include "block_predictor.h"
#include "dct_block_stream.h"
#include "dct_block_transformer.h"
#include "prediction_type_stream.h"

template <size_t D>
struct LFIFDecoder {
  LFIFHeader<D> header;

  void open(std::istream &input) {
    header.read(input);
  }

  template<typename F>
  void decodeStream(std::istream &input, F &&pusher) {
    DynamicBlock<float, D> block_Y(this->header.block_size);
    DynamicBlock<float, D> block_U(this->header.block_size);
    DynamicBlock<float, D> block_V(this->header.block_size);

    std::array<size_t, D> aligned_image_size {};
    for (size_t i = 0; i < D; i++) {
      aligned_image_size[i] = (this->header.size[i] + this->header.block_size[i] - 1) / this->header.block_size[i] * this->header.block_size[i];
    }

    IBitstream   bitstream {};
    CABACDecoder cabac { [&] { return bitstream.readBit(input); } };

    DCTCoefs<D> dct_coefs(this->header.block_size);

    DCTBlockStream<D> stream_Y(this->header.block_size);
    DCTBlockStream<D> stream_UV(this->header.block_size);

    std::array<size_t, D> predictor_size {};
    if (this->header.predicted) {
      predictor_size = this->header.size;
    }

    BlockPredictor<D, float> predictor_Y(predictor_size);
    BlockPredictor<D, float> predictor_U(predictor_size);
    BlockPredictor<D, float> predictor_V(predictor_size);

    PredictionTypeStream<D> pred_type_stream {};

    for (const auto &offset : block_for<D>({}, this->header.block_size, aligned_image_size)) {
      for (size_t i = 0; i < D; i++) {
        std::print(stderr, "{} ", offset[i]);
      }
      std::print(stderr, "out of ");
      for (size_t i = 0; i < D; i++) {
        std::print(stderr, "{} ", aligned_image_size[i]);
      }
      std::println(stderr);

      decode_dct_block(stream_Y,  block_Y, cabac);
      decode_dct_block(stream_UV, block_U, cabac);
      decode_dct_block(stream_UV, block_V, cabac);

      dct_inverse(block_Y, dct_coefs, this->header.discarded_bits);
      dct_inverse(block_U, dct_coefs, this->header.discarded_bits);
      dct_inverse(block_V, dct_coefs, this->header.discarded_bits);

      if (this->header.predicted) {
        auto prediction_type = decode_prediction_type(pred_type_stream, cabac);

        predictor_Y.backwardPass(block_Y, offset, prediction_type);
        predictor_U.backwardPass(block_U, offset, prediction_type);
        predictor_V.backwardPass(block_V, offset, prediction_type);
      }

      moveBlock<D>([&](const auto &pos) {
                     float Y  = block_Y[pos] + pow(2, this->header.depth_bits - 1);
                     float Cb = block_U[pos];
                     float Cr = block_V[pos];

                     uint16_t R = std::clamp<float>(std::round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, std::pow(2, this->header.depth_bits) - 1);
                     uint16_t G = std::clamp<float>(std::round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, std::pow(2, this->header.depth_bits) - 1);
                     uint16_t B = std::clamp<float>(std::round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, std::pow(2, this->header.depth_bits) - 1);

                     return std::array<uint16_t, 3>({R, G, B});
                   }, this->header.block_size, {},
                   pusher, this->header.size, offset,
                   this->header.block_size);
    }
  }
};
