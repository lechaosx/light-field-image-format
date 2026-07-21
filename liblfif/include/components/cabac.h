#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

// Shared CABAC constants and the per-decision context model.
struct CABAC {

  struct ContextModel {
    uint8_t m_state : 6;
    uint8_t m_mps   : 1;  // most probable symbol
  };

  static constexpr uint8_t  BITS    = 10;
  static constexpr uint16_t ONE     = static_cast<uint16_t>(1 << BITS);
  static constexpr uint16_t HALF    = static_cast<uint16_t>(1 << (BITS - 1));
  static constexpr uint16_t QUARTER = static_cast<uint16_t>(1 << (BITS - 2));

  static constexpr uint8_t rangeTabLPS[64][4] {
    {128, 176, 208, 240},
    {128, 167, 197, 227},
    {128, 158, 187, 216},
    {123, 150, 178, 205},
    {116, 142, 169, 195},
    {111, 135, 160, 185},
    {105, 128, 152, 175},
    {100, 122, 144, 166},
    { 95, 116, 137, 158},
    { 90, 110, 130, 150},
    { 85, 104, 123, 142},
    { 81,  99, 117, 135},
    { 77,  94, 111, 128},
    { 73,  89, 105, 122},
    { 69,  85, 100, 116},
    { 66,  80,  95, 110},
    { 62,  76,  90, 104},
    { 59,  72,  86,  99},
    { 56,  69,  81,  94},
    { 53,  65,  77,  89},
    { 51,  62,  73,  85},
    { 48,  59,  69,  80},
    { 46,  56,  66,  76},
    { 43,  53,  63,  72},
    { 41,  50,  59,  69},
    { 39,  48,  56,  65},
    { 37,  45,  54,  62},
    { 35,  43,  51,  59},
    { 33,  41,  48,  56},
    { 32,  39,  46,  53},
    { 30,  37,  43,  50},
    { 29,  35,  41,  48},
    { 27,  33,  39,  45},
    { 26,  31,  37,  43},
    { 24,  30,  35,  41},
    { 23,  28,  33,  39},
    { 22,  27,  32,  37},
    { 21,  26,  30,  35},
    { 20,  24,  29,  33},
    { 19,  23,  27,  31},
    { 18,  22,  26,  30},
    { 17,  21,  25,  28},
    { 16,  20,  23,  27},
    { 15,  19,  22,  25},
    { 14,  18,  21,  24},
    { 14,  17,  20,  23},
    { 13,  16,  19,  22},
    { 12,  15,  18,  21},
    { 12,  14,  17,  20},
    { 11,  14,  16,  19},
    { 11,  13,  15,  18},
    { 10,  12,  15,  17},
    { 10,  12,  14,  16},
    {  9,  11,  13,  15},
    {  9,  11,  12,  14},
    {  8,  10,  12,  14},
    {  8,   9,  11,  13},
    {  7,   9,  11,  12},
    {  7,   9,  10,  12},
    {  7,   8,  10,  11},
    {  6,   8,   9,  11},
    {  6,   7,   9,  10},
    {  6,   7,   8,   9},
    {  2,   2,   2,   2}
  };
  static constexpr uint8_t transIdxMPS[64] {
     1,  2,  3,  4,  5,  6,  7,  8,
     9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56,
    57, 58, 59, 60, 61, 62, 62, 63
  };
  static constexpr uint8_t transIdxLPS[64] {
     0,  0,  1,  2,  2,  4,  4,  5,
     6,  7,  8,  9,  9, 11, 11, 12,
    13, 13, 15, 15, 16, 16, 18, 18,
    19, 19, 21, 21, 22, 22, 23, 24,
    24, 25, 26, 26, 27, 27, 28, 29,
    29, 30, 30, 30, 31, 32, 32, 33,
    33, 33, 34, 34, 35, 35, 35, 36,
    36, 36, 37, 37, 37, 38, 38, 63
  };
};

// Arithmetic encoder. Emits coded bits to a caller-provided sink (a callable
// taking a bool); it never touches a stream itself.
template <typename Sink>
class CABACEncoder {
  uint16_t m_low         {};
  uint16_t m_range       { CABAC::HALF - 2 };
  size_t   m_outstanding {};
  Sink     m_sink;

  void putCabacBit(bool bit) {
    m_sink(bit);
    while (m_outstanding) {
      m_sink(!bit);
      m_outstanding--;
    }
  }

  void renormalize() {
    while (m_range < CABAC::QUARTER) {
      if (m_low >= CABAC::HALF) {
        putCabacBit(1);
        m_low -= CABAC::HALF;
      }
      else if (m_low < CABAC::QUARTER) {
        putCabacBit(0);
      }
      else {
        m_outstanding++;
        m_low -= CABAC::QUARTER;
      }

      m_low   <<= 1;
      m_range <<= 1;
    }
  }

public:
  explicit CABACEncoder(Sink sink): m_sink(std::move(sink)) {}

  void encodeBit(CABAC::ContextModel &context, bool bit) {
    uint8_t rLPS = CABAC::rangeTabLPS[context.m_state][(m_range >> 6) & 3];
    m_range -= rLPS;

    if (bit != context.m_mps) {
      m_low  += m_range;
      m_range = rLPS;

      if (context.m_state == 0) {
        context.m_mps = !context.m_mps;
      }

      context.m_state = CABAC::transIdxLPS[context.m_state];
    }
    else {
      context.m_state = CABAC::transIdxMPS[context.m_state];
    }

    renormalize();
  }

  void encodeBitBypass(bool bit) {
    m_low <<= 1;

    if (bit == true) {
      m_low += m_range;
    }

    if (m_low >= CABAC::ONE) {
      putCabacBit(1);
      m_low -= CABAC::ONE;
    }
    else if (m_low < CABAC::HALF) {
      putCabacBit(0);
    }
    else {
      m_outstanding++;
      m_low -= CABAC::HALF;
    }
  }

  void terminate() {
    m_low += 2;
    m_range = 2;

    renormalize();

    putCabacBit(0);
    putCabacBit(0);
  }

  void encodeU(CABAC::ContextModel &context, uint64_t value) {
    for (size_t i = 0; i < value; i++) {
      encodeBit(context, 1);
    }
    encodeBit(context, 0);
  }

  void encodeEG(uint64_t k, CABAC::ContextModel &context, uint64_t value) {
    while (value >= (uint64_t { 1 } << k)) {
      encodeBit(context, 1);
      value -= uint64_t { 1 } << k;
      k++;
    }

    encodeBit(context, 0);

    while (k--) {
      encodeBitBypass((value >> k) & 1);
    }
  }

  void encodeEGBypass(uint64_t k, uint64_t value) {
    while (value >= (uint64_t { 1 } << k)) {
      encodeBitBypass(1);
      value -= uint64_t { 1 } << k;
      k++;
    }

    encodeBitBypass(0);

    while (k--) {
      encodeBitBypass((value >> k) & 1);
    }
  }

  void encodeUEG0(uint64_t u_bits, CABAC::ContextModel &context, uint64_t value) {
    for (size_t i = 0; i < (value < u_bits ? value : u_bits); i++) {
      encodeBit(context, 1);
    }

    if (value < u_bits) {
      encodeBit(context, 0);
    }
    else {
      encodeEGBypass(0, value - u_bits);
    }
  }

  void encodeRG(uint64_t &k, CABAC::ContextModel &context, uint64_t value) {
    for (size_t i = 0; i < (value >> k); i++) {
      encodeBit(context, 1);
    }
    encodeBit(context, 0);

    for (size_t i = 1; i <= k; i++) {
      encodeBitBypass((value >> (k - i)) & 1);
    }

    if ((value > (uint64_t { 3 } << k)) && k < 4) {
      k++;
    }
  }
};

// Arithmetic decoder. Pulls coded bits from a caller-provided source (a
// callable returning a bool); it never touches a stream itself.
template <typename Source>
class CABACDecoder {
  uint16_t m_low   {};
  uint16_t m_range { CABAC::HALF - 2 };
  Source   m_source;

public:
  explicit CABACDecoder(Source source): m_source(std::move(source)) {
    for (size_t i = 0; i < CABAC::BITS; i++) {
      m_low = (m_low << 1) | m_source();
    }
  }

  bool decodeBit(CABAC::ContextModel &context) {
    uint8_t rLPS = CABAC::rangeTabLPS[context.m_state][(m_range >> 6) & 3];
    m_range -= rLPS;

    bool bit {};
    if (m_low < m_range) {
      bit             = context.m_mps;
      context.m_state = CABAC::transIdxMPS[context.m_state];
    }
    else {
      m_low  -= m_range;
      m_range = rLPS;
      bit     = !context.m_mps;

      if (context.m_state == 0) {
        context.m_mps = !context.m_mps;
      }

      context.m_state = CABAC::transIdxLPS[context.m_state];
    }

    while (m_range < CABAC::QUARTER) {
      m_range <<= 1;
      m_low = (m_low << 1) | m_source();
    }

    return bit;
  }

  bool decodeBitBypass() {
    m_low = (m_low << 1) | m_source();

    if (m_low >= m_range) {
      m_low -= m_range;
      return 1;
    }
    else {
      return 0;
    }
  }

  uint64_t decodeU(CABAC::ContextModel &context) {
    uint64_t value { 0 };
    while (decodeBit(context)) {
      value++;
    }
    return value;
  }

  uint64_t decodeUEG0(uint64_t u_bits, CABAC::ContextModel &context) {
    uint64_t value { 0 };

    bool bit = decodeBit(context);
    while (bit && (value < (u_bits - 1))) {
      value++;
      bit = decodeBit(context);
    }

    if (bit) {
      value += decodeEG(0) + 1;
    }

    return value;
  }

  uint64_t decodeEG(uint64_t k) {
    uint64_t value      { 0 };
    uint64_t bin_symbol { 0 };

    for (bool bit = decodeBitBypass(); bit; bit = decodeBitBypass()) {
      value += uint64_t { 1 } << k;
      k++;
    }

    while (k--) {
      bin_symbol |= uint64_t { decodeBitBypass() } << k;
    }

    value += bin_symbol;

    return value;
  }
};
