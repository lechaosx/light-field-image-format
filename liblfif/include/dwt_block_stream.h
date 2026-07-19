#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

#include "components/block.h"
#include "components/cabac.h"
#include "components/meta.h"

template<size_t D>
class DWTBlockStreamEncoder {
  CABAC::ContextModel significant_coef_flag_ctx {};
  CABAC::ContextModel coef_greater_one_ctx {};
  CABAC::ContextModel coef_greater_two_ctx {};
  CABAC::ContextModel coef_abs_level_ctx {};

public:

  void encodeBlock(const DynamicBlock<int32_t, D> &block, CABACEncoder &encoder) {
    for (const auto &pos : iterate_dimensions<D>(block.size())) {
      int32_t coef = block[pos];

      encoder.encodeBit(significant_coef_flag_ctx, coef);

      if (coef) {
        bool sign {};

        if (coef > 0) {
          sign = 0;
        }
        else {
          sign = 1;
          coef = -coef;
        }

        encoder.encodeBit(coef_greater_one_ctx, coef > 1);

        if (coef > 1) {
          encoder.encodeBit(coef_greater_two_ctx, coef > 2);

          if (coef > 2) {
            encoder.encodeU(coef_abs_level_ctx, coef - 3);
          }
        }

        encoder.encodeBitBypass(sign);
      }
    }
  }
};


template<size_t D>
class DWTBlockStreamDecoder {
  CABAC::ContextModel significant_coef_flag_ctx {};
  CABAC::ContextModel coef_greater_one_ctx {};
  CABAC::ContextModel coef_greater_two_ctx {};
  CABAC::ContextModel coef_abs_level_ctx {};

public:

  void decodeBlock(CABACDecoder &decoder, DynamicBlock<int32_t, D> &block) {
    for (const auto &pos : iterate_dimensions<D>(block.size())) {
      int32_t coef = decoder.decodeBit(significant_coef_flag_ctx);

      if (coef) {
        coef += decoder.decodeBit(coef_greater_one_ctx);

        if (coef > 1) {
          coef += decoder.decodeBit(coef_greater_two_ctx);

          if (coef > 2) {
            coef += decoder.decodeU(coef_abs_level_ctx);
          }
        }

        if (decoder.decodeBitBypass()) {
          coef = -coef;
        }
      }

      block[pos] = coef;
    }
  }
};
