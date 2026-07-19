#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

#include "components/block.h"
#include "components/cabac.h"
#include "components/colorspace.h"
#include "components/meta.h"
#include "components/dct.h"
#include "components/diag_scan.h"

template<size_t D>
struct DCTCompressedBlockStreamState {
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

  DCTCompressedBlockStreamState(const std::array<size_t, D> &block_size) {
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
class DCTBlockStreamEncoder {
  DCTCompressedBlockStreamState<D> state;

public:

  DCTBlockStreamEncoder(const std::array<size_t, D> &block_size): state(block_size) {}

  void encodeBlock(const DynamicBlock<float, D> &block, CABACEncoder &encoder) {
    std::vector<bool> nonzero_diags(state.diagonals);

    for (size_t diag = 0; diag < state.diagonals; diag++) {
      for (const auto &pos : diagonalScan(state.block_size, diag)) {
        if (block[pos]) {
          nonzero_diags[diag] = true;
        }
      }
    }

    size_t diags_cnt = 0;
    for (size_t diag = 0; diag < state.diagonals; diag++) {
      if (nonzero_diags[diag]) {
        diags_cnt++;
      }
    }

    for (size_t diag = 0; diag < state.diagonals; diag++) {
      encoder.encodeBit(state.coded_diag_flag_ctx[diag], nonzero_diags[diag]);

      if (nonzero_diags[diag]) {
        diags_cnt--;

        if (diags_cnt) {
          encoder.encodeBit(state.last_coded_diag_flag_ctx[diag], 0);
        }
        else {
          encoder.encodeBit(state.last_coded_diag_flag_ctx[diag], 1);
          break;
        }
      }
    }

    for (size_t d = 1; d <= state.diagonals; d++) {
      size_t diag = state.diagonals - d;
      int64_t zero_coef_distr = 0;

      if (nonzero_diags[diag]) {
        for (auto pos : diagonalScan(state.block_size, diag)) {
          int32_t coef = block[pos];

          size_t nonzero_neighbours_cnt = 0;

          for (size_t dim = 0; dim < D; dim++) {
            pos[dim]++;
            if (pos[dim] < state.block_size[dim]) {
              nonzero_neighbours_cnt += (block[pos] != 0);
            }
            pos[dim]--;
          }

          if (diag < state.threshold) {
            encoder.encodeBit(state.significant_coef_flag_high_ctx[nonzero_neighbours_cnt], coef);
          }
          else {
            encoder.encodeBit(state.significant_coef_flag_low_ctx[nonzero_neighbours_cnt], coef);
          }

          if (!coef) {
            zero_coef_distr++;
          }
          else {
            zero_coef_distr--;

            bool sign {};

            if (coef > 0) {
              sign = 0;
            }
            else {
              sign = 1;
              coef = -coef;
            }

            encoder.encodeBit(state.coef_greater_one_ctx[diag], coef > 1);

            if (coef > 1) {
              encoder.encodeBit(state.coef_greater_two_ctx[diag], coef > 2);

              if (coef > 2) {
                encoder.encodeU(state.coef_abs_level_ctx[diag], coef - 3);
              }
            }

            encoder.encodeBitBypass(sign);
          }
        }
      }

      if ((zero_coef_distr > 0) && (state.threshold > diag)) {
        state.threshold--;
      }
      else if ((zero_coef_distr < 0) && (state.threshold < diag)) {
        state.threshold++;
      }
    }
  }
};


template<size_t D>
class DCTBlockStreamDecoder {
  DCTCompressedBlockStreamState<D> state;

public:

  DCTBlockStreamDecoder(const std::array<size_t, D> &block_size): state(block_size) {}

  void decodeBlock(CABACDecoder &decoder, DynamicBlock<float, D> &block) {
    block.fill(0.f);
    std::vector<bool> nonzero_diags(state.diagonals);

    for (size_t diag = 0; diag < state.diagonals; diag++) {
      nonzero_diags[diag] = decoder.decodeBit(state.coded_diag_flag_ctx[diag]);

      if (nonzero_diags[diag]) {
        if (decoder.decodeBit(state.last_coded_diag_flag_ctx[diag])) {
          break;
        }
      }
    }

    for (size_t d = 1; d <= state.diagonals; d++) {
      size_t diag = state.diagonals - d;
      int64_t zero_coef_distr = 0;

      if (nonzero_diags[diag]) {
        for (auto pos : diagonalScan(state.block_size, diag)) {
          int32_t coef = 0;

          size_t nonzero_neighbours_cnt = 0;

          for (size_t dim = 0; dim < D; dim++) {
            pos[dim]++;
            if (pos[dim] < state.block_size[dim]) {
              nonzero_neighbours_cnt += (block[pos] != 0);
            }
            pos[dim]--;
          }

          if (diag < state.threshold) {
            coef = decoder.decodeBit(state.significant_coef_flag_high_ctx[nonzero_neighbours_cnt]);
          }
          else {
            coef = decoder.decodeBit(state.significant_coef_flag_low_ctx[nonzero_neighbours_cnt]);
          }

          if (!coef) {
            zero_coef_distr++;
          }
          else {
            zero_coef_distr--;

            coef += decoder.decodeBit(state.coef_greater_one_ctx[diag]);

            if (coef > 1) {
              coef += decoder.decodeBit(state.coef_greater_two_ctx[diag]);

              if (coef > 2) {
                coef += decoder.decodeU(state.coef_abs_level_ctx[diag]);
              }
            }

            if (decoder.decodeBitBypass()) {
              coef = -coef;
            }
          }

          block[pos] = coef;
        }
      }

      if ((zero_coef_distr > 0) && (state.threshold > diag)) {
        state.threshold--;
      }
      else if ((zero_coef_distr < 0) && (state.threshold < diag)) {
        state.threshold++;
      }
    }
  }
};
