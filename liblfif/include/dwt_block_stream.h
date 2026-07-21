#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

#include "components/block.h"
#include "components/cabac.h"
#include "components/meta.h"

template<size_t D>
struct DWTBlockStream {
  CABAC::ContextModel significant_coef_flag_ctx {};
  CABAC::ContextModel coef_greater_one_ctx {};
  CABAC::ContextModel coef_greater_two_ctx {};
  CABAC::ContextModel coef_abs_level_ctx {};
};

template<size_t D>
void encode_dwt_block(DWTBlockStream<D> &s, const DynamicBlock<int32_t, D> &block, auto &encoder) {
  for (const auto &pos : iterate_dimensions<D>(block.size())) {
    int32_t coef = block[pos];

    encoder.encodeBit(s.significant_coef_flag_ctx, coef);

    if (coef) {
      bool sign = coef < 0;
      if (sign) coef = -coef;

      encoder.encodeBit(s.coef_greater_one_ctx, coef > 1);

      if (coef > 1) {
        encoder.encodeBit(s.coef_greater_two_ctx, coef > 2);

        if (coef > 2) {
          encoder.encodeU(s.coef_abs_level_ctx, coef - 3);
        }
      }

      encoder.encodeBitBypass(sign);
    }
  }
}

template<size_t D>
void decode_dwt_block(DWTBlockStream<D> &s, DynamicBlock<int32_t, D> &block, auto &decoder) {
  for (const auto &pos : iterate_dimensions<D>(block.size())) {
    int32_t coef = decoder.decodeBit(s.significant_coef_flag_ctx);

    if (coef) {
      coef += decoder.decodeBit(s.coef_greater_one_ctx);

      if (coef > 1) {
        coef += decoder.decodeBit(s.coef_greater_two_ctx);

        if (coef > 2) {
          coef += decoder.decodeU(s.coef_abs_level_ctx);
        }
      }

      if (decoder.decodeBitBypass()) {
        coef = -coef;
      }
    }

    block[pos] = coef;
  }
}
