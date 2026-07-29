#pragma once

#include "components/block.h"
#include "components/cabac.h"
#include "components/colorspace.h"
#include "components/meta.h"
#include "components/dct.h"
#include "components/diag_scan.h"

#include <cstdint>
#include <cstddef>

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

template<size_t D>
class DCTCompressedBlockStreamState {
protected:
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

    diagonals = num_diagonals<D>(block_size);

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
class DCTBlockStreamEncoder: public DCTCompressedBlockStreamState<D> {
public:

  DCTBlockStreamEncoder(const std::array<size_t, D> &block_size): DCTCompressedBlockStreamState<D>(block_size) {}

  void encodeBlock(const DynamicBlock<float, D> &block, CABACEncoder &encoder) {
    std::vector<bool> nonzero_diags(this->diagonals);

    for (size_t diag = 0; diag < this->diagonals; diag++) {
      diagonalScan(this->block_size, diag, [&](const std::array<size_t, D> &pos) {
        if (block[pos]) {
          nonzero_diags[diag] = true;
        }
      });
    }

    size_t diags_cnt = 0;
    for (size_t diag = 0; diag < this->diagonals; diag++) {
      if (nonzero_diags[diag]) {
        diags_cnt++;
      }
    }

    for (size_t diag = 0; diag < this->diagonals; diag++) {
      encoder.encodeBit(this->coded_diag_flag_ctx[diag], nonzero_diags[diag]);

      if (nonzero_diags[diag]) {
        diags_cnt--;

        if (diags_cnt) {
          encoder.encodeBit(this->last_coded_diag_flag_ctx[diag], 0);
        }
        else {
          encoder.encodeBit(this->last_coded_diag_flag_ctx[diag], 1);
          break;
        }
      }
    }

    for (size_t d = 1; d <= this->diagonals; d++) {
      size_t diag = this->diagonals - d;
      int64_t zero_coef_distr = 0;

      if (nonzero_diags[diag]) {
        diagonalScan(this->block_size, diag, [&](std::array<size_t, D> pos) {
          const double coefficient = block[pos];
          constexpr double maximum_coefficient =
              std::numeric_limits<int32_t>::max();
          if (!std::isfinite(coefficient)
              || coefficient < -maximum_coefficient
              || coefficient > maximum_coefficient) {
            throw std::overflow_error(
                "coefficient magnitude exceeds codec limit");
          }
          const int32_t integer_coefficient =
              static_cast<int32_t>(coefficient);
          const int64_t signed_magnitude = integer_coefficient;
          const uint64_t magnitude = signed_magnitude < 0
              ? static_cast<uint64_t>(-signed_magnitude)
              : static_cast<uint64_t>(signed_magnitude);

          size_t nonzero_neighbours_cnt = 0;

          for (size_t dim = 0; dim < D; dim++) {
            pos[dim]++;
            if (pos[dim] < this->block_size[dim]) {
              nonzero_neighbours_cnt += (block[pos] != 0);
            }
            pos[dim]--;
          }

          if (diag < this->threshold) {
            encoder.encodeBit(
                this->significant_coef_flag_high_ctx[nonzero_neighbours_cnt],
                magnitude != 0);
          }
          else {
            encoder.encodeBit(
                this->significant_coef_flag_low_ctx[nonzero_neighbours_cnt],
                magnitude != 0);
          }

          if (!magnitude) {
            zero_coef_distr++;
          }
          else {
            zero_coef_distr--;

            encoder.encodeBit(
                this->coef_greater_one_ctx[diag], magnitude > 1);

            if (magnitude > 1) {
              encoder.encodeBit(
                  this->coef_greater_two_ctx[diag], magnitude > 2);

              if (magnitude > 2) {
                encoder.encodeEG(
                    0, this->coef_abs_level_ctx[diag], magnitude - 3);
              }
            }

            encoder.encodeBitBypass(integer_coefficient < 0);
          }
        });
      }

      if ((zero_coef_distr > 0) && (this->threshold > diag)) {
        this->threshold--;
      }
      else if ((zero_coef_distr < 0) && (this->threshold < diag)) {
        this->threshold++;
      }
    }
  }
};


template<size_t D>
class DCTBlockStreamDecoder: public DCTCompressedBlockStreamState<D> {
public:

  DCTBlockStreamDecoder(const std::array<size_t, D> &block_size): DCTCompressedBlockStreamState<D>(block_size) {}

  void decodeBlock(CABACDecoder &decoder, DynamicBlock<float, D> &block) {
    constexpr uint64_t maximum_coefficient =
        static_cast<uint64_t>(std::numeric_limits<int32_t>::max());

    block.fill(0.f);
    std::vector<bool> nonzero_diags(this->diagonals);

    for (size_t diag = 0; diag < this->diagonals; diag++) {
      nonzero_diags[diag] = decoder.decodeBit(this->coded_diag_flag_ctx[diag]);

      if (nonzero_diags[diag]) {
        if (decoder.decodeBit(this->last_coded_diag_flag_ctx[diag])) {
          break;
        }
      }
    }

    for (size_t d = 1; d <= this->diagonals; d++) {
      size_t diag = this->diagonals - d;
      int64_t zero_coef_distr = 0;

      if (nonzero_diags[diag]) {
        diagonalScan<D>(this->block_size, diag, [&](std::array<size_t, D> pos) {
          uint64_t magnitude {};
          bool negative {};

          size_t nonzero_neighbours_cnt = 0;

          for (size_t dim = 0; dim < D; dim++) {
            pos[dim]++;
            if (pos[dim] < this->block_size[dim]) {
              nonzero_neighbours_cnt += (block[pos] != 0);
            }
            pos[dim]--;
          }

          if (diag < this->threshold) {
            magnitude = decoder.decodeBit(
                this->significant_coef_flag_high_ctx[nonzero_neighbours_cnt]);
          }
          else {
            magnitude = decoder.decodeBit(
                this->significant_coef_flag_low_ctx[nonzero_neighbours_cnt]);
          }

          if (!magnitude) {
            zero_coef_distr++;
          }
          else {
            zero_coef_distr--;

            magnitude += decoder.decodeBit(this->coef_greater_one_ctx[diag]);
            if (magnitude > maximum_coefficient) {
              throw std::runtime_error("coefficient magnitude exceeds codec limit");
            }

            if (magnitude > 1) {
              magnitude += decoder.decodeBit(this->coef_greater_two_ctx[diag]);
              if (magnitude > maximum_coefficient) {
                throw std::runtime_error("coefficient magnitude exceeds codec limit");
              }

              if (magnitude > 2) {
                magnitude += decoder.decodeEG(
                    0, this->coef_abs_level_ctx[diag],
                    maximum_coefficient - magnitude);
              }
            }

            negative = decoder.decodeBitBypass();
          }

          const int32_t coefficient = static_cast<int32_t>(magnitude);
          block[pos] = negative ? -coefficient : coefficient;
        });
      }

      if ((zero_coef_distr > 0) && (this->threshold > diag)) {
        this->threshold--;
      }
      else if ((zero_coef_distr < 0) && (this->threshold < diag)) {
        this->threshold++;
      }
    }
  }
};
