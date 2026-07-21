#pragma once

#include <cstdint>
#include <print>

#include "components/bitstream.h"
#include "components/lfif_header.h"

#include "block_predictor.h"
#include "dwt_block_stream.h"
#include "dwt_block_transformer.h"
#include "prediction_type_stream.h"

template <size_t D>
struct LFWFEncoder {
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
    DynamicBlock<int32_t, D> block_Y(this->header.block_size);
    DynamicBlock<int32_t, D> block_U(this->header.block_size);
    DynamicBlock<int32_t, D> block_V(this->header.block_size);

    DWTBlockStream<D> stream_Y {};
    DWTBlockStream<D> stream_UV {};

    std::array<size_t, D> predictor_size {};
    if (this->header.predicted) {
      predictor_size = this->header.size;
    }

    BlockPredictor<D, int32_t> predictor_Y(predictor_size);
    BlockPredictor<D, int32_t> predictor_U(predictor_size);
    BlockPredictor<D, int32_t> predictor_V(predictor_size);

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
                     block_Y[block_pos] = ((value[0] + (value[1] << 1) + value[2]) >> 2) - (1 << (this->header.depth_bits - 1));
                     block_U[block_pos] = value[2] - value[1];
                     block_V[block_pos] = value[0] - value[1];
                   }, this->header.block_size, {},
                   this->header.block_size);

      PredictionType<D> prediction_type {};
      if (this->header.predicted) {
        prediction_type = predictor_Y.selectPredictionType(block_Y, offset);

        predictor_Y.forwardPass(block_Y, offset, prediction_type);
        predictor_U.forwardPass(block_U, offset, prediction_type);
        predictor_V.forwardPass(block_V, offset, prediction_type);
      }

      dwt_forward(block_Y, this->header.discarded_bits);
      dwt_forward(block_U, this->header.discarded_bits);
      dwt_forward(block_V, this->header.discarded_bits);

      encode_dwt_block(stream_Y,  block_Y, cabac);
      encode_dwt_block(stream_UV, block_U, cabac);
      encode_dwt_block(stream_UV, block_V, cabac);

      if (this->header.predicted) {
        dwt_inverse(block_Y, this->header.discarded_bits);
        dwt_inverse(block_U, this->header.discarded_bits);
        dwt_inverse(block_V, this->header.discarded_bits);

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
