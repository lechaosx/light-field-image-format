#pragma once

#include "components/block.h"
#include "components/cabac.h"
#include "components/colorspace.h"
#include "components/meta.h"
#include "components/dct.h"
#include "components/diag_scan.h"

#include "lfif.h"

#include <cstdint>
#include <cstddef>

#include <array>

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
  uint8_t discarded_bits;

  DCTCompressedBlockStreamState(const std::array<size_t, D> &block_size, uint8_t discarded_bits) {
    this->block_size = block_size;
    this->discarded_bits = discarded_bits;

    diagonals = num_diagonals<D>(block_size);

    threshold = diagonals / 2;

    coded_diag_flag_ctx.resize(diagonals);
    last_coded_diag_flag_ctx.resize(diagonals);
    significant_coef_flag_high_ctx.resize(D + 1);
    significant_coef_flag_low_ctx.resize(D + 1);
    coef_greater_one_ctx.resize(D + 1);
    coef_greater_two_ctx.resize(D + 1);
    coef_abs_level_ctx.resize(D + 1);
  }
};

template<size_t D>
class DCTBlockStreamEncoder: public DCTCompressedBlockStreamState<D> {
public:

  DCTBlockStreamEncoder(const std::array<size_t, D> &block_size, uint8_t discarded_bits): DCTCompressedBlockStreamState<D>(block_size, discarded_bits) {}

  void encodeBlock(DynamicBlock<float, D> &block, CABACEncoder &encoder) {
    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    fdct<D>(this->block_size, proxy);

    iterate_dimensions<D>(this->block_size, [&](const auto &pos) {
      block[pos] = std::round(ldexp(block[pos], -this->discarded_bits)); // QUANTIZATION
    });

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
          int32_t coef = block[pos];

          size_t nonzero_neighbours_cnt = 0;

          for (size_t dim = 0; dim < D; dim++) {
            pos[dim]++;
            if (pos[dim] < this->block_size[dim]) {
              nonzero_neighbours_cnt += (block[pos] != 0);
            }
            pos[dim]--;
          }

          if (diag < this->threshold) {
            encoder.encodeBit(this->significant_coef_flag_high_ctx[nonzero_neighbours_cnt], coef);
          }
          else {
            encoder.encodeBit(this->significant_coef_flag_low_ctx[nonzero_neighbours_cnt], coef);
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

            encoder.encodeBit(this->coef_greater_one_ctx[nonzero_neighbours_cnt], coef > 1);

            if (coef > 1) {
              encoder.encodeBit(this->coef_greater_two_ctx[nonzero_neighbours_cnt], coef > 2);

              if (coef > 2) {
                encoder.encodeU(this->coef_abs_level_ctx[nonzero_neighbours_cnt], coef - 3);
              }
            }

            encoder.encodeBitBypass(sign);
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

  void decodeEncodedBlock(DynamicBlock<float, D> &block) {
    iterate_dimensions<D>(this->block_size, [&](const auto &pos) {
      block[pos] = ldexp(block[pos], this->discarded_bits);
    });

    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    idct<D>(block.size(), proxy);
  }
};


template<size_t D>
class DCTBlockStreamDecoder: public DCTCompressedBlockStreamState<D> {
public:

  DCTBlockStreamDecoder(const std::array<size_t, D> &block_size, uint8_t discarded_bits): DCTCompressedBlockStreamState<D>(block_size, discarded_bits) {}

  void decodeEncodedBlock(DynamicBlock<float, D> &block) {
    iterate_dimensions<D>(this->block_size, [&](const auto &pos) {
      block[pos] = ldexp(block[pos], this->discarded_bits);
    });

    auto proxy = [&](size_t index) -> auto & {
      return block[index];
    };

    idct<D>(block.size(), proxy);
  }

  void decodeBlock(CABACDecoder &decoder, DynamicBlock<float, D> &block) {
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
          int32_t coef = 0.f;

          size_t nonzero_neighbours_cnt = 0;

          for (size_t dim = 0; dim < D; dim++) {
            pos[dim]++;
            if (pos[dim] < this->block_size[dim]) {
              nonzero_neighbours_cnt += (block[pos] != 0);
            }
            pos[dim]--;
          }

          if (diag < this->threshold) {
            coef = decoder.decodeBit(this->significant_coef_flag_high_ctx[nonzero_neighbours_cnt]);
          }
          else {
            coef = decoder.decodeBit(this->significant_coef_flag_low_ctx[nonzero_neighbours_cnt]);
          }

          if (!coef) {
            zero_coef_distr++;
          }
          else {
            zero_coef_distr--;

            coef += decoder.decodeBit(this->coef_greater_one_ctx[nonzero_neighbours_cnt]);

            if (coef > 1) {
              coef += decoder.decodeBit(this->coef_greater_two_ctx[nonzero_neighbours_cnt]);

              if (coef > 2) {
                coef += decoder.decodeU(this->coef_abs_level_ctx[nonzero_neighbours_cnt]);
              }
            }

            if (decoder.decodeBitBypass()) {
              coef = -coef;
            }
          }

          block[pos] = coef;
        });
      }

      if ((zero_coef_distr > 0) && (this->threshold > diag)) {
        this->threshold--;
      }
      else if ((zero_coef_distr < 0) && (this->threshold < diag)) {
        this->threshold++;
      }
    }

    decodeEncodedBlock(block);
  }
};
