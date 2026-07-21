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
struct LFIFEncoder {
  LFIFHeader<D> header;

  void create(
            std::ostream          &output,
      const std::array<size_t, D> &size,
      const std::array<size_t, D> &block_size,
            uint8_t                depth_bits,
            uint8_t                discarded_bits,
            bool                   predicted) {

    header = { size, block_size, depth_bits, discarded_bits, predicted };
    header.write(output);
  }

  template <typename F>
  void encodeStream(F &&puller, std::ostream &output) {
    DynamicBlock<float, D> block_Y(this->header.block_size);
    DynamicBlock<float, D> block_U(this->header.block_size);
    DynamicBlock<float, D> block_V(this->header.block_size);

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

    OBitstream   bitstream {};
    CABACEncoder cabac { [&](bool bit) { bitstream.writeBit(output, bit); } };

    std::array<size_t, D> aligned_image_size {};
    for (size_t i = 0; i < D; i++) {
      aligned_image_size[i] = (this->header.size[i] + this->header.block_size[i] - 1) / this->header.block_size[i] * this->header.block_size[i];
    }

    for (const auto &offset : block_for<D>({}, this->header.block_size, aligned_image_size)) {
      for (size_t i = 0; i < D; i++) {
        std::print(stderr, "{} ", offset[i]);
      }
      std::print(stderr, "out of ");
      for (size_t i = 0; i < D; i++) {
        std::print(stderr, "{} ", aligned_image_size[i]);
      }
      std::println(stderr);

      moveBlock<D>(puller, this->header.size, offset,
                   [&](const auto &block_pos, const auto &value) {
                     block_Y[block_pos] = YCbCr::RGBToY(value[0], value[1], value[2]) - pow(2, this->header.depth_bits - 1);
                     block_U[block_pos] = YCbCr::RGBToCb(value[0], value[1], value[2]);
                     block_V[block_pos] = YCbCr::RGBToCr(value[0], value[1], value[2]);
                   }, this->header.block_size, {},
                   this->header.block_size);

      PredictionType<D> prediction_type {};
      if (this->header.predicted) {
        prediction_type = predictor_Y.selectPredictionType(block_Y, offset);

        predictor_Y.forwardPass(block_Y, offset, prediction_type);
        predictor_U.forwardPass(block_U, offset, prediction_type);
        predictor_V.forwardPass(block_V, offset, prediction_type);
      }

      dct_forward(block_Y, dct_coefs, this->header.discarded_bits);
      dct_forward(block_U, dct_coefs, this->header.discarded_bits);
      dct_forward(block_V, dct_coefs, this->header.discarded_bits);

      encode_dct_block(stream_Y,  block_Y, cabac);
      encode_dct_block(stream_UV, block_U, cabac);
      encode_dct_block(stream_UV, block_V, cabac);

      if (this->header.predicted) {
        dct_inverse(block_Y, dct_coefs, this->header.discarded_bits);
        dct_inverse(block_U, dct_coefs, this->header.discarded_bits);
        dct_inverse(block_V, dct_coefs, this->header.discarded_bits);

        predictor_Y.backwardPass(block_Y, offset, prediction_type);
        predictor_U.backwardPass(block_U, offset, prediction_type);
        predictor_V.backwardPass(block_V, offset, prediction_type);

        encode_prediction_type(pred_type_stream, prediction_type, cabac);
      }
    }

    cabac.terminate();
    bitstream.flush(output);
  }
};
