#pragma once

#include "components/bitstream.h"
#include "components/colorspace.h"

#include "block_predictor.h"
#include "dct_block_stream.h"
#include "dct_block_transformer.h"
#include "prediction_type_stream.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>

template <size_t D>
struct LFIFEncoder {
  std::array<size_t, D> size;
  std::array<size_t, D> block_size;
  uint8_t depth_bits;
  uint8_t discarded_bits;
  bool predicted;

  template <typename F>
  void encodeStream(F &&puller, std::ostream &output) {
    DynamicBlock<float, D> block_Y(this->block_size);
    DynamicBlock<float, D> block_U(this->block_size);
    DynamicBlock<float, D> block_V(this->block_size);

    DCTCoefs<D> dct_coefs(this->block_size);

    DCTBlockStream<D> block_stream_Y(this->block_size);
    DCTBlockStream<D> block_stream_UV(this->block_size);

    std::array<size_t, D> predictor_size {};
    if (this->predicted) {
      predictor_size = this->size;
    }

    BlockPredictor<D, float> predictor_Y(predictor_size);
    BlockPredictor<D, float> predictor_U(predictor_size);
    BlockPredictor<D, float> predictor_V(predictor_size);

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

      dctForward(block_Y, dct_coefs, this->discarded_bits);
      dctForward(block_U, dct_coefs, this->discarded_bits);
      dctForward(block_V, dct_coefs, this->discarded_bits);

      encodeDctBlock(block_stream_Y, block_Y, cabac);
      encodeDctBlock(block_stream_UV, block_U, cabac);
      encodeDctBlock(block_stream_UV, block_V, cabac);

      if (this->predicted) {
        dctInverse(block_Y, dct_coefs, this->discarded_bits);
        dctInverse(block_U, dct_coefs, this->discarded_bits);
        dctInverse(block_V, dct_coefs, this->discarded_bits);

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
