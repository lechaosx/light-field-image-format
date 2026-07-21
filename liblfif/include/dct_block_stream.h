#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

#include "components/block.h"
#include "components/cabac.h"
#include "components/meta.h"
#include "components/dct.h"
#include "components/diag_scan.h"

template<size_t D>
struct DCTBlockStream {
  size_t diagonals;
  size_t threshold;

  std::vector<CABAC::ContextModel> significant_coef_flag_high_ctx;
  std::vector<CABAC::ContextModel> significant_coef_flag_low_ctx;
  std::vector<CABAC::ContextModel> coded_diag_flag_ctx;
  std::vector<CABAC::ContextModel> last_coded_diag_flag_ctx;
  std::vector<CABAC::ContextModel> coef_greater_one_ctx;
  std::vector<CABAC::ContextModel> coef_greater_two_ctx;
  std::vector<CABAC::ContextModel> coef_abs_level_ctx;

  std::array<size_t, D> block_size;

  DCTBlockStream(const std::array<size_t, D> &block_size) {
    this->block_size = block_size;
    diagonals = num_diagonals(block_size);
    threshold = diagonals / 2;
    coded_diag_flag_ctx.resize(diagonals);
    last_coded_diag_flag_ctx.resize(diagonals);
    significant_coef_flag_high_ctx.resize(D + 1);
    significant_coef_flag_low_ctx.resize(D + 1);
    coef_greater_one_ctx.resize(diagonals);
    coef_greater_two_ctx.resize(diagonals);
    coef_abs_level_ctx.resize(diagonals);
  }
};

template<size_t D>
void encode_dct_block(DCTBlockStream<D> &s, const DynamicBlock<float, D> &block, auto &encoder) {
  std::vector<bool> nonzero_diags(s.diagonals);

  for (size_t diag = 0; diag < s.diagonals; diag++) {
    for (const auto &pos : diagonalScan(s.block_size, diag)) {
      if (block[pos]) {
        nonzero_diags[diag] = true;
      }
    }
  }

  size_t diags_cnt = 0;
  for (size_t diag = 0; diag < s.diagonals; diag++) {
    if (nonzero_diags[diag]) {
      diags_cnt++;
    }
  }

  for (size_t diag = 0; diag < s.diagonals; diag++) {
    encoder.encodeBit(s.coded_diag_flag_ctx[diag], nonzero_diags[diag]);

    if (nonzero_diags[diag]) {
      diags_cnt--;

      if (diags_cnt) {
        encoder.encodeBit(s.last_coded_diag_flag_ctx[diag], 0);
      }
      else {
        encoder.encodeBit(s.last_coded_diag_flag_ctx[diag], 1);
        break;
      }
    }
  }

  for (size_t d = 1; d <= s.diagonals; d++) {
    size_t diag = s.diagonals - d;
    int64_t zero_coef_distr = 0;

    if (nonzero_diags[diag]) {
      for (auto pos : diagonalScan(s.block_size, diag)) {
        int32_t coef = block[pos];

        size_t nonzero_neighbours_cnt = 0;

        for (size_t dim = 0; dim < D; dim++) {
          pos[dim]++;
          if (pos[dim] < s.block_size[dim]) {
            nonzero_neighbours_cnt += (block[pos] != 0);
          }
          pos[dim]--;
        }

        if (diag < s.threshold) {
          encoder.encodeBit(s.significant_coef_flag_high_ctx[nonzero_neighbours_cnt], coef);
        }
        else {
          encoder.encodeBit(s.significant_coef_flag_low_ctx[nonzero_neighbours_cnt], coef);
        }

        if (!coef) {
          zero_coef_distr++;
        }
        else {
          zero_coef_distr--;

          bool sign = coef < 0;
          if (sign) coef = -coef;

          encoder.encodeBit(s.coef_greater_one_ctx[diag], coef > 1);

          if (coef > 1) {
            encoder.encodeBit(s.coef_greater_two_ctx[diag], coef > 2);

            if (coef > 2) {
              encoder.encodeU(s.coef_abs_level_ctx[diag], coef - 3);
            }
          }

          encoder.encodeBitBypass(sign);
        }
      }
    }

    if ((zero_coef_distr > 0) && (s.threshold > diag)) {
      s.threshold--;
    }
    else if ((zero_coef_distr < 0) && (s.threshold < diag)) {
      s.threshold++;
    }
  }
}

template<size_t D>
void decode_dct_block(DCTBlockStream<D> &s, DynamicBlock<float, D> &block, auto &decoder) {
  block.fill(0.f);
  std::vector<bool> nonzero_diags(s.diagonals);

  for (size_t diag = 0; diag < s.diagonals; diag++) {
    nonzero_diags[diag] = decoder.decodeBit(s.coded_diag_flag_ctx[diag]);

    if (nonzero_diags[diag]) {
      if (decoder.decodeBit(s.last_coded_diag_flag_ctx[diag])) {
        break;
      }
    }
  }

  for (size_t d = 1; d <= s.diagonals; d++) {
    size_t diag = s.diagonals - d;
    int64_t zero_coef_distr = 0;

    if (nonzero_diags[diag]) {
      for (auto pos : diagonalScan(s.block_size, diag)) {
        int32_t coef = 0;

        size_t nonzero_neighbours_cnt = 0;

        for (size_t dim = 0; dim < D; dim++) {
          pos[dim]++;
          if (pos[dim] < s.block_size[dim]) {
            nonzero_neighbours_cnt += (block[pos] != 0);
          }
          pos[dim]--;
        }

        if (diag < s.threshold) {
          coef = decoder.decodeBit(s.significant_coef_flag_high_ctx[nonzero_neighbours_cnt]);
        }
        else {
          coef = decoder.decodeBit(s.significant_coef_flag_low_ctx[nonzero_neighbours_cnt]);
        }

        if (!coef) {
          zero_coef_distr++;
        }
        else {
          zero_coef_distr--;

          coef += decoder.decodeBit(s.coef_greater_one_ctx[diag]);

          if (coef > 1) {
            coef += decoder.decodeBit(s.coef_greater_two_ctx[diag]);

            if (coef > 2) {
              coef += decoder.decodeU(s.coef_abs_level_ctx[diag]);
            }
          }

          if (decoder.decodeBitBypass()) {
            coef = -coef;
          }
        }

        block[pos] = coef;
      }
    }

    if ((zero_coef_distr > 0) && (s.threshold > diag)) {
      s.threshold--;
    }
    else if ((zero_coef_distr < 0) && (s.threshold < diag)) {
      s.threshold++;
    }
  }
}
