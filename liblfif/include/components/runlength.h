/**
* @file runlength.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for performing run-length encoding.
*/

#pragma once

#include <bit>
#include <cstdint>
#include <algorithm>

#include "huffman.h"
#include "cabac.h"
#include "endian.h"

using RLAMPUNIT = int64_t;    /**< @brief Unit intended to contain amplitude value in un-length pair.*/
using HuffmanClass = uint8_t; /**< @brief Unit intended to contain number of bits of amplitude.*/

/**
 * @brief Class which describes one run-length pair.
 */
class RunLengthPair {
public:
  size_t    zeroes;    /**< @brief Number of subsequent zeroes in one run-length.*/
  RLAMPUNIT amplitude; /**< @brief The non-zero amplitude. */

  /**
   * @brief Method which encodes the pair to stream by Huffman encoder.
   * @param encoder The Huffman encoder.
   * @param stream The stream to which the symbol shall be encoded.
   * @param class_bits Minimum number of bits needed to contain the maximum possible amplitude size.
   */
  void huffmanEncodeToStream(const HuffmanEncoder &encoder, OBitstream &stream, size_t class_bits) const;

  /**
   * @brief Method which decodes the pair from stream by Huffman decoder.
   * @param decoder The Huffman decoder.
   * @param stream The stream from which the symbol shall be decoded.
   * @param class_bits Minimum number of bits needed to contain the maximum possible amplitude size.
   */
  void huffmanDecodeFromStream(const HuffmanDecoder &decoder, IBitstream &stream, size_t class_bits);

  /**
   * @brief Method which adds pair to the weight map.
   * @param weights Weight map.
   * @param class_bits Minimum number of bits needed to contain the maximum possible amplitude size.
   */
   void addToWeights(HuffmanWeights &weights, size_t class_bits) const {
    weights[huffmanSymbol(class_bits)]++;
  }

  /**
   * @brief Checks if the pair is EOB.
   * @return True if EOB, false else.
   */
  bool eob() const {
    return (!zeroes) && (!amplitude);
  }

  /**
   * @brief Computes number of bits from symbol will be used for zeroes.
   * @param class_bits Minimum number of bits needed to contain the maximum possible amplitude size.
   * @return Number of bits.
   */
  static size_t zeroesBits(size_t class_bits) {
    return sizeof(HuffmanSymbol) * 8 - class_bits;
  }

  /**
   * @brief Computes number of bits from symbol will be used for amplitude class.
   * @param amp_bits Minimum number of bits needed to contain the maximum possible amplitude.
   * @return Minimum number of bits needed to contain the maximum possible amplitude size.
   */
  static size_t classBits(size_t amp_bits) {
    return std::bit_width(amp_bits);
  }

  HuffmanClass huffmanClass() const {
    uint64_t abs_amp = amplitude < 0 ? static_cast<uint64_t>(-amplitude) : static_cast<uint64_t>(amplitude);
    return std::bit_width(abs_amp);
  }

  HuffmanSymbol huffmanSymbol(size_t class_bits) const {
    return zeroes << class_bits | huffmanClass();
  }

};
