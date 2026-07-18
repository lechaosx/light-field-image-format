/**
* @file cabac.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 11. 7. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Context-Adaptive Binary Arithmetic Coding.
*/

#pragma once

#include "bitstream.h"

/**
* @brief Base CABAC state class.
*/
class CABAC {
public:

  /**
  * @brief Structure which contains probabilitis of one binary decision.
  */
  struct ContextModel {
    uint8_t m_state : 6;  /**< @brief State of the CABAC context model.*/
    uint8_t m_mps   : 1;  /**< @brief Most probable symbol (0|1).*/
  };

protected:
  uint16_t m_low;
  uint16_t m_range;

  static constexpr uint8_t  BITS    = 10;
  static constexpr uint16_t ONE     = static_cast<uint16_t>(1 << BITS);
  static constexpr uint16_t HALF    = static_cast<uint16_t>(1 << (BITS - 1));
  static constexpr uint16_t QUARTER = static_cast<uint16_t>(1 << (BITS - 2));

  const uint8_t rangeTabLPS[64][4] {
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
  const uint8_t transIdxMPS[64] {
     1,  2,  3,  4,  5,  6,  7,  8,
     9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56,
    57, 58, 59, 60, 61, 62, 62, 63
  };
  const uint8_t transIdxLPS[64] {
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

class CABACEncoder: public CABAC {
public:

  /**
  * @brief Function which initializes the encoder.
  * @param stream Bitstream to which the data will be written.
  */
  inline void init(OBitstream &stream);

  /**
  * @brief Function changes stream for the encoder.
  * @param stream Bitstream to which the data will be written.
  */
  inline void setStream(OBitstream &stream);

  /**
  * @brief Function which encodes one bit.
  * @param context Context specifiing probabilities of binary decision.
  * @param bit Value that is encoded.
  */
  inline void encodeBit(ContextModel &context, bool bit);

  /**
  * @brief Function which encodes one bit with 50/50 probability.
  * @param bit Value that is encoded.
  */
  inline void encodeBitBypass(bool bit);

  /**
  * @brief Function which terminates the encoding.
  */
  inline void terminate();

  inline void encodeU(ContextModel &context, uint64_t value);
  inline void encodeEG(uint64_t k, ContextModel &context, uint64_t value);
  inline void encodeEGBypass(uint64_t k, uint64_t value);
  inline void encodeUEG0(uint64_t u_bits, ContextModel &context, uint64_t value);
  inline void encodeRG(uint64_t &k, ContextModel &context, uint64_t value);

private:
  OBitstream *m_stream;
  size_t      m_outstanding;

  inline void renormalize();
  inline void putCabacBit(bool bit);
};

class CABACDecoder: public CABAC {
public:
  /**
  * @brief Function which initializes the encoder.
  * @param stream Bitstream from which the data will be read.
  */
  inline void init(IBitstream &stream);

  /**
  * @brief Function which decodes one bit.
  * @param context Context specifiing probabilities of binary decision.
  * @return Decoded binary value.
  */
  inline bool decodeBit(ContextModel &context);

  /**
  * @brief Function which decodes one bit which was encoded with 50/50 probability.
  * @return Decoded binary value.
  */
  inline bool decodeBitBypass();

  /**
  * @brief Function which terminates the decoding.
  */
  inline void terminate();

  inline uint64_t decodeU(ContextModel &context);
  inline uint64_t decodeUEG0(uint64_t u_bits, ContextModel &context);
  inline uint64_t decodeEG(uint64_t k);

private:
  IBitstream *m_stream;
};

void CABACEncoder::init(OBitstream &stream) {
  m_low         = 0;
  m_range       = HALF - 2;
  m_outstanding = 0;
  m_stream      = &stream;
}

void CABACEncoder::setStream(OBitstream &stream) {
  m_stream      = &stream;
}

void CABACEncoder::encodeBit(ContextModel &context, bool bit) {
  uint8_t rLPS {};

  rLPS = rangeTabLPS[context.m_state][(m_range >> 6) & 3];
  m_range -= rLPS;

  if (bit != context.m_mps) {
    m_low  += m_range;
    m_range = rLPS;

    if (context.m_state == 0) {
      context.m_mps = !context.m_mps;
    }

    context.m_state = transIdxLPS[context.m_state];
  }
  else {
    context.m_state = transIdxMPS[context.m_state];
  }

  renormalize();
}

void CABACEncoder::encodeBitBypass(bool bit) {
  m_low <<= 1;

  if (bit == true) {
    m_low += m_range;
  }

  if (m_low >= ONE) {
    putCabacBit(1);
    m_low -= ONE;
  }
  else if (m_low < HALF) {
    putCabacBit(0);
  }
  else {
    m_outstanding++;
    m_low -= HALF;
  }
}

void CABACEncoder::terminate() {
  m_low += 2;
  m_range = 2;

  renormalize();

  putCabacBit(0);
  putCabacBit(0);

  m_stream->flush();

  m_stream = nullptr;
}

void CABACEncoder::renormalize() {
  while (m_range < QUARTER) {
    if (m_low >= HALF) {
      putCabacBit(1);
      m_low -= HALF;
    }
    else if (m_low < QUARTER) {
      putCabacBit(0);
    }
    else {
      m_outstanding++;
      m_low -= QUARTER;
    }

    m_low   <<= 1;
    m_range <<= 1;
  }
}

void CABACEncoder::putCabacBit(bool bit) {
  m_stream->writeBit(bit);

  while (m_outstanding) {
    m_stream->writeBit(!bit);
    m_outstanding--;
  }
}

void CABACEncoder::encodeU(ContextModel &context, uint64_t value) {
  for (size_t i = 0; i < value; i++) {
    encodeBit(context, 1);
  }
  encodeBit(context, 0);
}

void CABACEncoder::encodeEG(uint64_t k, ContextModel &context, uint64_t value) {
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

void CABACEncoder::encodeEGBypass(uint64_t k, uint64_t value) {
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

void CABACEncoder::encodeUEG0(uint64_t u_bits, ContextModel &context, uint64_t value) {
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

void CABACEncoder::encodeRG(uint64_t &k, ContextModel &context, uint64_t value) {
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

void CABACDecoder::init(IBitstream &stream) {
  m_low    = 0;
  m_range  = HALF - 2;
  m_stream = &stream;

  for (size_t i = 0; i < BITS; i++) {
    m_low = (m_low << 1) | m_stream->readBit();
  }
}

bool CABACDecoder::decodeBit(ContextModel &context) {
  bool    bit  {};
  uint8_t rLPS {};

  rLPS     = rangeTabLPS[context.m_state][(m_range >> 6) & 3];
  m_range -= rLPS;

  if (m_low < m_range) {
    bit             = context.m_mps;
    context.m_state = transIdxMPS[context.m_state];
  }
  else {
    m_low  -= m_range;
    m_range = rLPS;
    bit     = !context.m_mps;

    if (context.m_state == 0) {
      context.m_mps = !context.m_mps;
    }

    context.m_state = transIdxLPS[context.m_state];
  }

  while (m_range < QUARTER) {
    m_range <<= 1;
    m_low = (m_low << 1) | m_stream->readBit();
  }

  return bit;
}

bool CABACDecoder::decodeBitBypass() {
  m_low = (m_low << 1) | m_stream->readBit();

  if (m_low >= m_range) {
    m_low -= m_range;
    return 1;
  }
  else {
    return 0;
  }
}

void CABACDecoder::terminate() {
  m_stream = nullptr;
}

uint64_t CABACDecoder::decodeU(ContextModel &context) {
  uint64_t value { 0 };
  while (decodeBit(context)) {
    value++;
  }
  return value;
}

uint64_t CABACDecoder::decodeUEG0(uint64_t u_bits, ContextModel &context) {
  uint64_t value { 0 };
  bool bit       {};

  bit = decodeBit(context);
  while (bit && (value < (u_bits - 1))) {
    value++;
    bit = decodeBit(context);
  }

  if (bit) {
    value += decodeEG(0) + 1;
  }

  return value;
}

uint64_t CABACDecoder::decodeEG(uint64_t k) {
  uint64_t value      { 0 };
  uint64_t bin_symbol { 0 };

  for (bool bit = decodeBitBypass(); bit; bit = decodeBitBypass()) {
    value += uint64_t { 1 } << k;
    k++;
  }

  while (k--) {
    bin_symbol |= uint64_t { decodeBitBypass() } << k;
  };

  value += bin_symbol;

  return value;
}
