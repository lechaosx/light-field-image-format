#pragma once

#include "components/bitstream.h"
#include "components/colorspace.h"

#include "block_predictor.h"
#include "dwt_block_transformer.h"
#include "dwt_block_stream.h"
#include "prediction_type_stream.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>

template <size_t D>
struct LFWFDecoder {
  std::array<size_t, D> size;
  std::array<size_t, D> block_size;
  uint8_t depth_bits;
  uint8_t discarded_bits;
  bool predicted;

  template<typename F>
  void decodeStream(std::istream &input, F &&pusher) {
    DynamicBlock<int32_t, D> block_Y(this->block_size);
    DynamicBlock<int32_t, D> block_U(this->block_size);
    DynamicBlock<int32_t, D> block_V(this->block_size);

    std::array<size_t, D> aligned_image_size {};
    for (size_t i = 0; i < D; i++) {
      const size_t blocks =
          this->size[i] / this->block_size[i]
          + (this->size[i] % this->block_size[i] != 0);
      aligned_image_size[i] = blocks * this->block_size[i];
    }

    IBitstream bitstream([&]() -> uint8_t {
      const auto value = input.get();
      if (value == std::istream::traits_type::eof()) {
        throw std::ios_base::failure(
            input.eof()
                ? "unexpected end of bitstream"
                : "failed to read bitstream");
      }
      return static_cast<uint8_t>(value);
    });
    CABACDecoder cabac([&] { return bitstream.readBit(); });

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

    for (const auto &offset :
         block_for<D>({}, this->block_size, aligned_image_size)) {
      decodeDwtBlock(block_stream_Y, cabac, block_Y);
      decodeDwtBlock(block_stream_UV, cabac, block_U);
      decodeDwtBlock(block_stream_UV, cabac, block_V);

      dwtInverse(block_Y, this->discarded_bits);
      dwtInverse(block_U, this->discarded_bits);
      dwtInverse(block_V, this->discarded_bits);

      if (this->predicted) {
        auto prediction_type =
            decodePredictionType(prediction_type_stream, cabac);

        predictor_Y.backwardPass(block_Y, offset, prediction_type);
        predictor_U.backwardPass(block_U, offset, prediction_type);
        predictor_V.backwardPass(block_V, offset, prediction_type);
      }

      moveBlock<D>([&](const auto &pos) {
                     const int64_t Y =
                         static_cast<int64_t>(block_Y[pos])
                         + (int64_t {1} << (this->depth_bits - 1));
                     const int64_t U = block_U[pos];
                     const int64_t V = block_V[pos];
                     const int64_t maximum =
                         (int64_t {1} << this->depth_bits) - 1;

                     uint16_t G = std::clamp<int64_t>(
                         Y - ((U + V) >> 2), 0, maximum);
                     uint16_t R = std::clamp<int64_t>(
                         V + G, 0, maximum);
                     uint16_t B = std::clamp<int64_t>(
                         U + G, 0, maximum);

                     return std::array<uint16_t, 3>({R, G, B});
                   }, this->block_size, {},
                   pusher, this->size, offset,
                   this->block_size);
    }

  }
};
