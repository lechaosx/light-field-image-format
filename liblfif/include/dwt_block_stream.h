#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

#include "components/block.h"
#include "components/cabac.h"

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
      int32_t coef = block[pos];

      encoder.encodeBit(this->significant_coef_flag_ctx, coef);

      if (coef) {
        bool sign {};

        if (coef > 0) {
          sign = 0;
        }
        else {
          sign = 1;
          coef = -coef;
        }

        encoder.encodeBit(this->coef_greater_one_ctx, coef > 1);

        if (coef > 1) {
          encoder.encodeBit(this->coef_greater_two_ctx, coef > 2);

          if (coef > 2) {
            encoder.encodeU(this->coef_abs_level_ctx, coef - 3);
          }
        }

        encoder.encodeBitBypass(sign);
      }
    });
  }
};


template<size_t D>
class DWTBlockStreamDecoder: public DWTCompressedBlockStreamState<D> {
public:

  void decodeBlock(CABACDecoder &decoder, DynamicBlock<int32_t, D> &block) {
    iterate_dimensions<D>(block.size(), [&](const auto &pos) {
      int32_t coef = decoder.decodeBit(this->significant_coef_flag_ctx);

      if (coef) {
        coef += decoder.decodeBit(this->coef_greater_one_ctx);

        if (coef > 1) {
          coef += decoder.decodeBit(this->coef_greater_two_ctx);

          if (coef > 2) {
            coef += decoder.decodeU(this->coef_abs_level_ctx);
          }
        }

        if (decoder.decodeBitBypass()) {
          coef = -coef;
        }
      }

      block[pos] = coef;
    });
  }
};
