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

  explicit DCTBlockStream(const std::array<size_t, D> &size)
      : diagonals(num_diagonals<D>(size)),
        threshold(diagonals / 2),
        significant_coef_flag_high_ctx(D + 1),
        significant_coef_flag_low_ctx(D + 1),
        coded_diag_flag_ctx(diagonals),
        last_coded_diag_flag_ctx(diagonals),
        coef_greater_one_ctx(diagonals),
        coef_greater_two_ctx(diagonals),
        coef_abs_level_ctx(diagonals),
        block_size(size) {}
};

template<size_t D>
void encodeDctBlock(
    DCTBlockStream<D> &stream,
    const DynamicBlock<float, D> &block,
    auto &encoder) {
    std::vector<bool> nonzero_diags(stream.diagonals);

    for (size_t diag = 0; diag < stream.diagonals; diag++) {
      for (const auto &pos : diagonalScan(stream.block_size, diag)) {
        if (block[pos]) {
          nonzero_diags[diag] = true;
        }
      }
    }

    size_t diags_cnt = 0;
    for (size_t diag = 0; diag < stream.diagonals; diag++) {
      if (nonzero_diags[diag]) {
        diags_cnt++;
      }
    }

    for (size_t diag = 0; diag < stream.diagonals; diag++) {
      encoder.encodeBit(stream.coded_diag_flag_ctx[diag], nonzero_diags[diag]);

      if (nonzero_diags[diag]) {
        diags_cnt--;

        if (diags_cnt) {
          encoder.encodeBit(stream.last_coded_diag_flag_ctx[diag], 0);
        }
        else {
          encoder.encodeBit(stream.last_coded_diag_flag_ctx[diag], 1);
          break;
        }
      }
    }

    for (size_t d = 1; d <= stream.diagonals; d++) {
      size_t diag = stream.diagonals - d;
      int64_t zero_coef_distr = 0;

      if (nonzero_diags[diag]) {
        for (auto pos : diagonalScan(stream.block_size, diag)) {
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
            if (pos[dim] < stream.block_size[dim]) {
              nonzero_neighbours_cnt += (block[pos] != 0);
            }
            pos[dim]--;
          }

          if (diag < stream.threshold) {
            encoder.encodeBit(
                stream.significant_coef_flag_high_ctx[nonzero_neighbours_cnt],
                magnitude != 0);
          }
          else {
            encoder.encodeBit(
                stream.significant_coef_flag_low_ctx[nonzero_neighbours_cnt],
                magnitude != 0);
          }

          if (!magnitude) {
            zero_coef_distr++;
          }
          else {
            zero_coef_distr--;

            encoder.encodeBit(
                stream.coef_greater_one_ctx[diag], magnitude > 1);

            if (magnitude > 1) {
              encoder.encodeBit(
                  stream.coef_greater_two_ctx[diag], magnitude > 2);

              if (magnitude > 2) {
                encoder.encodeEG(
                    0, stream.coef_abs_level_ctx[diag], magnitude - 3);
              }
            }

            encoder.encodeBitBypass(integer_coefficient < 0);
          }
        }
      }

      if ((zero_coef_distr > 0) && (stream.threshold > diag)) {
        stream.threshold--;
      }
      else if ((zero_coef_distr < 0) && (stream.threshold < diag)) {
        stream.threshold++;
      }
    }
}


template<size_t D>
void decodeDctBlock(
    DCTBlockStream<D> &stream,
    auto &decoder,
    DynamicBlock<float, D> &block) {
    constexpr uint64_t maximum_coefficient =
        static_cast<uint64_t>(std::numeric_limits<int32_t>::max());

    block.fill(0.f);
    std::vector<bool> nonzero_diags(stream.diagonals);

    for (size_t diag = 0; diag < stream.diagonals; diag++) {
      nonzero_diags[diag] = decoder.decodeBit(stream.coded_diag_flag_ctx[diag]);

      if (nonzero_diags[diag]) {
        if (decoder.decodeBit(stream.last_coded_diag_flag_ctx[diag])) {
          break;
        }
      }
    }

    for (size_t d = 1; d <= stream.diagonals; d++) {
      size_t diag = stream.diagonals - d;
      int64_t zero_coef_distr = 0;

      if (nonzero_diags[diag]) {
        for (auto pos : diagonalScan(stream.block_size, diag)) {
          uint64_t magnitude {};
          bool negative {};

          size_t nonzero_neighbours_cnt = 0;

          for (size_t dim = 0; dim < D; dim++) {
            pos[dim]++;
            if (pos[dim] < stream.block_size[dim]) {
              nonzero_neighbours_cnt += (block[pos] != 0);
            }
            pos[dim]--;
          }

          if (diag < stream.threshold) {
            magnitude = decoder.decodeBit(
                stream.significant_coef_flag_high_ctx[nonzero_neighbours_cnt]);
          }
          else {
            magnitude = decoder.decodeBit(
                stream.significant_coef_flag_low_ctx[nonzero_neighbours_cnt]);
          }

          if (!magnitude) {
            zero_coef_distr++;
          }
          else {
            zero_coef_distr--;

            magnitude += decoder.decodeBit(stream.coef_greater_one_ctx[diag]);
            if (magnitude > maximum_coefficient) {
              throw std::runtime_error("coefficient magnitude exceeds codec limit");
            }

            if (magnitude > 1) {
              magnitude += decoder.decodeBit(stream.coef_greater_two_ctx[diag]);
              if (magnitude > maximum_coefficient) {
                throw std::runtime_error("coefficient magnitude exceeds codec limit");
              }

              if (magnitude > 2) {
                magnitude += decoder.decodeEG(
                    0, stream.coef_abs_level_ctx[diag],
                    maximum_coefficient - magnitude);
              }
            }

            negative = decoder.decodeBitBypass();
          }

          const int32_t coefficient = static_cast<int32_t>(magnitude);
          block[pos] = negative ? -coefficient : coefficient;
        }
      }

      if ((zero_coef_distr > 0) && (stream.threshold > diag)) {
        stream.threshold--;
      }
      else if ((zero_coef_distr < 0) && (stream.threshold < diag)) {
        stream.threshold++;
      }
    }
}
