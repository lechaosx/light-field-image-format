#pragma once

#include "components/block.h"
#include "components/cabac.h"

#include <cstdint>
#include <cstddef>

#include <array>
#include <limits>
#include <stdexcept>

template<size_t D>
class DWTCompressedBlockStreamState {
protected:
  CABAC::ContextModel significant_coef_flag_ctx;
  CABAC::ContextModel coef_greater_one_ctx;
  CABAC::ContextModel coef_greater_two_ctx;
  CABAC::ContextModel coef_abs_level_ctx;
};

template<size_t D>
class DWTBlockStreamEncoder: public DWTCompressedBlockStreamState<D> {
public:

  void encodeBlock(const DynamicBlock<int32_t, D> &block, CABACEncoder &encoder) {
    iterate_dimensions<D>(block.size(), [&](const auto &pos) {
      const int64_t coefficient = block[pos];
      const uint64_t magnitude = coefficient < 0
          ? static_cast<uint64_t>(-coefficient)
          : static_cast<uint64_t>(coefficient);
      if (magnitude > static_cast<uint64_t>(
              std::numeric_limits<int32_t>::max())) {
        throw std::overflow_error("coefficient magnitude exceeds codec limit");
      }

      encoder.encodeBit(this->significant_coef_flag_ctx, magnitude != 0);

      if (magnitude) {
        encoder.encodeBit(this->coef_greater_one_ctx, magnitude > 1);

        if (magnitude > 1) {
          encoder.encodeBit(this->coef_greater_two_ctx, magnitude > 2);

          if (magnitude > 2) {
            encoder.encodeEG(
                0, this->coef_abs_level_ctx, magnitude - 3);
          }
        }

        encoder.encodeBitBypass(coefficient < 0);
      }
    });
  }
};


template<size_t D>
class DWTBlockStreamDecoder: public DWTCompressedBlockStreamState<D> {
public:

  void decodeBlock(CABACDecoder &decoder, DynamicBlock<int32_t, D> &block) {
    constexpr uint64_t maximum_coefficient =
        static_cast<uint64_t>(std::numeric_limits<int32_t>::max());

    iterate_dimensions<D>(block.size(), [&](const auto &pos) {
      uint64_t magnitude =
          decoder.decodeBit(this->significant_coef_flag_ctx);
      bool negative {};

      if (magnitude) {
        magnitude += decoder.decodeBit(this->coef_greater_one_ctx);
        if (magnitude > maximum_coefficient) {
          throw std::runtime_error("coefficient magnitude exceeds codec limit");
        }

        if (magnitude > 1) {
          magnitude += decoder.decodeBit(this->coef_greater_two_ctx);
          if (magnitude > maximum_coefficient) {
            throw std::runtime_error("coefficient magnitude exceeds codec limit");
          }

          if (magnitude > 2) {
            magnitude += decoder.decodeEG(
                0, this->coef_abs_level_ctx,
                maximum_coefficient - magnitude);
          }
        }

        negative = decoder.decodeBitBypass();
      }

      const int32_t coefficient = static_cast<int32_t>(magnitude);
      block[pos] = negative ? -coefficient : coefficient;
    });
  }
};
