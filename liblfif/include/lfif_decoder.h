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
#include "prediction_type_stream.h"
#include "lfif.h"

#include <cstdint>

template <size_t D>
struct LFIFDecoder: public LFIF<D> {
  LFIFDecoder(): LFIF<D>() {
    StackAllocator::init(2147483648 * 4); //FIXME
  }

  ~LFIFDecoder() {
    StackAllocator::cleanup();
  }

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
    DynamicBlock<float, D> block_Y(this->block_size);
    DynamicBlock<float, D> block_U(this->block_size);
    DynamicBlock<float, D> block_V(this->block_size);

    std::array<size_t, D> aligned_image_size {};
    for (size_t i = 0; i < D; i++) {
      aligned_image_size[i] = (this->size[i] + this->block_size[i] - 1) / this->block_size[i] * this->block_size[i];
    }

    IBitstream   bitstream {};
    CABACDecoder cabac     {};

    LFIFBlockEncoder block_decoder_Y(this->block_size, this->discarded_bits);
    LFIFBlockEncoder block_decoder_UV(this->block_size, this->discarded_bits);

    BlockPredictor predictorY(this->size);
    BlockPredictor predictorU(this->size);
    BlockPredictor predictorV(this->size);

    PredictionTypeDecoder<D> prediction_type_decoder {};

    bitstream.open(input);
    cabac.init(bitstream);

    block_for<D>({}, this->block_size, aligned_image_size, [&](const std::array<size_t, D> &offset) {
      for (size_t i = 0; i < D; i++) {
        std::cerr << offset[i] << " ";
      }
      std::cerr << " out of ";
      for (size_t i = 0; i < D; i++) {
        std::cerr << aligned_image_size[i] << " ";
      }
      std::cerr << "\n";

      block_decoder_Y.decodeBlock(cabac, block_Y);
      block_decoder_UV.decodeBlock(cabac, block_U);
      block_decoder_UV.decodeBlock(cabac, block_V);

      if (this->predicted) {
        auto prediction_type = prediction_type_decoder.decodePredictionType(cabac);

        predictorY.backwardPass(block_Y, offset, prediction_type);
        predictorU.backwardPass(block_U, offset, prediction_type);
        predictorV.backwardPass(block_V, offset, prediction_type);
      }

      moveBlock<D>([&](const auto &pos) {
        float Y  = block_Y[pos] + pow(2, this->depth_bits - 1);
        float Cb = block_U[pos];
        float Cr = block_V[pos];

        uint16_t R = std::clamp<float>(std::round(YCbCr::YCbCrToR(Y, Cb, Cr)), 0, std::pow(2, this->depth_bits) - 1);
        uint16_t G = std::clamp<float>(std::round(YCbCr::YCbCrToG(Y, Cb, Cr)), 0, std::pow(2, this->depth_bits) - 1);
        uint16_t B = std::clamp<float>(std::round(YCbCr::YCbCrToB(Y, Cb, Cr)), 0, std::pow(2, this->depth_bits) - 1);

        return std::array<uint16_t, 3>({R, G, B});
      }, this->block_size, {},
      pusher, this->size, offset,
      this->block_size);
    });

    cabac.terminate();
  }
};
