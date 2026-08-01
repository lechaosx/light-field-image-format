#pragma once

#include "components/bitstream.h"

#include "block_predictor.h"
#include "dwt_block_stream.h"
#include "dwt_block_transformer.h"
#include "prediction_type_stream.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>

template <size_t D>
struct LFWFEncoder {
  std::array<size_t, D> size;
  std::array<size_t, D> block_size;
  uint8_t depth_bits;
  uint8_t discarded_bits;
  bool predicted;

  template <typename F>
  void encodeStream(F &&puller, std::ostream &output) {
    DynamicBlock<int32_t, D> block_Y(this->block_size);
    DynamicBlock<int32_t, D> block_U(this->block_size);
    DynamicBlock<int32_t, D> block_V(this->block_size);

    DWTBlockStream<D> block_stream_Y {};
    DWTBlockStream<D> block_stream_UV {};

    std::array<size_t, D> predictor_size {};
    if (this->predicted) {
      predictor_size = this->size;
    }

    BlockPredictor<D, int32_t> predictor_Y(predictor_size);
    BlockPredictor<D, int32_t> predictor_U(predictor_size);
    BlockPredictor<D, int32_t> predictor_V(predictor_size);

    PredictionTypeStream<D> prediction_type_stream {};

    OBitstream bitstream([&](uint8_t byte) {
      output.put(static_cast<char>(byte));
      if (!output) {
        throw std::ios_base::failure("failed to write bitstream");
      }
    });
    CABACEncoder cabac([&](bool bit) { bitstream.writeBit(bit); });

    std::array<size_t, D> aligned_image_size {};
    for (size_t i = 0; i < D; i++) {
      const size_t blocks =
          this->size[i] / this->block_size[i]
          + (this->size[i] % this->block_size[i] != 0);
      aligned_image_size[i] = blocks * this->block_size[i];
    }

    for (const auto &offset :
         block_for<D>({}, this->block_size, aligned_image_size)) {
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

      dwtForward(block_Y, this->discarded_bits);
      dwtForward(block_U, this->discarded_bits);
      dwtForward(block_V, this->discarded_bits);

      encodeDwtBlock(block_stream_Y, block_Y, cabac);
      encodeDwtBlock(block_stream_UV, block_U, cabac);
      encodeDwtBlock(block_stream_UV, block_V, cabac);

      if (this->predicted) {
        dwtInverse(block_Y, this->discarded_bits);
        dwtInverse(block_U, this->discarded_bits);
        dwtInverse(block_V, this->discarded_bits);

        predictor_Y.backwardPass(block_Y, offset, prediction_type);
        predictor_U.backwardPass(block_U, offset, prediction_type);
        predictor_V.backwardPass(block_V, offset, prediction_type);

        encodePredictionType(prediction_type_stream, prediction_type, cabac);
      }
    }

    cabac.terminate();
    bitstream.flush();
  }
};
