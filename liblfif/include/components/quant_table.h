#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <vector>

#include "dct.h"
#include "endian.h"
#include "block.h"

using DCTDATAUNIT = float;
using QTABLEUNIT = uint64_t;

template <size_t D>
using QuantTable = DynamicBlock<QTABLEUNIT, D>;

// JPEG quality-50 luma table.
static constexpr std::array<QTABLEUNIT, 64> base_luma {
  16,  11,  10,  16,  24,  40,  51,  61,
  12,  12,  14,  19,  26,  58,  60,  55,
  14,  13,  16,  24,  40,  57,  69,  56,
  14,  17,  22,  29,  51,  87,  80,  62,
  18,  22,  37,  56,  68, 109, 103,  77,
  24,  35,  55,  64,  81, 104, 113,  92,
  49,  64,  78,  87, 103, 121, 120, 101,
  72,  92,  95,  98, 112, 100, 103,  99
};

// JPEG quality-50 chroma table.
static constexpr std::array<QTABLEUNIT, 64> base_chroma {
  17, 18, 24, 47, 99, 99, 99, 99,
  18, 21, 26, 66, 99, 99, 99, 99,
  24, 26, 56, 99, 99, 99, 99, 99,
  47, 66, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99
};

inline QuantTable<2> baseLuma() {
  QuantTable<2> output({8, 8});
  for (size_t i {}; i < 64; i++) {
    output[i] = base_luma[i];
  }
  return output;
}

inline QuantTable<2> baseChroma() {
  QuantTable<2> output({8, 8});
  for (size_t i {}; i < 64; i++) {
    output[i] = base_chroma[i];
  }
  return output;
}

template <size_t D>
constexpr void scaleFillNear(const QuantTable<D> &input, QuantTable<D> &output) {
  auto inputF = [&](const std::array<size_t, D> &pos) {
    return input[pos];
  };

  auto outputF = [&](const std::array<size_t, D> &pos, const auto &value) {
    output[pos] = value;
  };

  std::array<size_t, D> pos {};
  getBlock<D>(output.size().data(), inputF, pos, input.size(), outputF);
}

template <size_t D>
constexpr void scaleByDCT(const QuantTable<D> &input, QuantTable<D> &output) {
  DynamicBlock<DCTDATAUNIT, D> input_coefs(input.size());
  DynamicBlock<DCTDATAUNIT, D> output_coefs(output.size());

  auto fInputF = [&](size_t index) -> DCTDATAUNIT {
    return input[index];
  };

  auto fOutputF = [&](size_t index) -> DCTDATAUNIT & {
    return input_coefs[index];
  };

  fdct<D>(input.size().data(), fInputF, fOutputF);

  auto iInputF = [&](size_t index) -> DCTDATAUNIT {
    size_t real_index {};

    for (size_t i { 1 }; i <= D; i++) {
      size_t dim = index % output.stride(D - i + 1) / output.stride(D - i);
      if (dim >= input.size()[D - i]) {
        return 0;
      }
      else {
        real_index *= input.size()[D - i];
        real_index += dim;
      }
    }

    return input_coefs[real_index];
  };

  auto iOutputF = [&](size_t index) -> DCTDATAUNIT & {
    return output_coefs[index];
  };

  idct<D>(output.size().data(), iInputF, iOutputF);

  for (size_t i = 0; i < output.stride(D); i++) {
    output[i] = std::round(output_coefs[i]);
  }
}

template <size_t D>
void applyQualityCoefficient(QuantTable<D> &table, float scale_coef) {
  for (size_t i = 0; i < table.stride(D); i++) {
    table[i] = std::round(table[i] * scale_coef);
  }
}

template <size_t D>
void applyQuality(QuantTable<D> &input, float quality) {
  float scale_coef = quality < 50 ? (50.0 / quality) : (200.0 - 2 * quality) / 100;
  applyQualityCoefficient<D>(input, scale_coef);
}

template <size_t D>
void uniformTable(QTABLEUNIT value, QuantTable<D> &output) {
  output.fill(value);
}

template <size_t DIN, size_t DOUT>
void copyTable(const QuantTable<DIN> &input, QuantTable<DOUT> &output) {
  static_assert(DIN <= DOUT);

  for (size_t i {}; i < output.stride(DOUT); i++) {
    output[i] = input[i % input.stride(DIN)];
  }
}

template <size_t DIN, size_t DOUT>
void averageDiagonalTable(const QuantTable<DIN> &input, QuantTable<DOUT> &output) {
  std::vector<double> diagonals_sum(num_diagonals<DIN>(input.size()));
  std::vector<size_t> diagonals_cnt(num_diagonals<DIN>(input.size()));

  for (const auto &pos : iterate_dimensions<DIN>(input.size())) {
    size_t diagonal {};
    for (size_t i = 0; i < DIN; i++) {
      diagonal += pos[i];
    }

    diagonals_sum[diagonal] += input[pos];
    diagonals_cnt[diagonal]++;
  }

  for (const auto &pos : iterate_dimensions<DOUT>(output.size())) {
    size_t diagonal {};
    for (size_t i = 0; i < DIN; i++) {
      diagonal += pos[i];
    }

    if (diagonal >= num_diagonals<DIN>(input.size())) {
      diagonal = num_diagonals<DIN>(input.size()) - 1;
    }

    output[pos] = std::round(diagonals_sum[diagonal] / diagonals_cnt[diagonal]);
  }
}

template <size_t D>
void clampTable(QuantTable<D> &table, float min, float max) {
  for (size_t i {}; i < table.stride(D); i++) {
    table[i] = std::clamp<float>(table[i], min, max);
  }
}

template <size_t D>
void writeQuantToStream(const QuantTable<D> &table, std::ostream &stream) {
  for (size_t i {}; i < table.stride(D); i++) {
    writeValueToStream<uint8_t>(table[i], stream);
  }
}

template <size_t D>
QuantTable<D> readQuantFromStream(const std::array<size_t, D> &BS, std::istream &stream) {
  QuantTable<D> table(BS);
  for (size_t i {}; i < table.stride(D); i++) {
    table[i] = readValueFromStream<uint8_t>(stream);
  }
  return table;
}
