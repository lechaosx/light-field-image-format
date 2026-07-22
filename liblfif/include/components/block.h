/**
* @file block.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Functions for block extraction and insertion.
*/

#pragma once

#include <cmath>

#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

template<typename T, size_t D>
class DynamicBlock {

  std::array<size_t, D>  m_size;
  std::vector<T>         m_data;

public:
  DynamicBlock(const std::array<size_t, D> &size): DynamicBlock(size.data()) {}

  #pragma omp declare simd
  DynamicBlock(const size_t size[D]) {
    std::copy(size, size + D, std::begin(m_size));
    size_t values = 1;
    for (const size_t extent : m_size) {
      if (extent != 0 && values > std::numeric_limits<size_t>::max() / extent) {
        throw std::length_error("block dimensions overflow");
      }
      values *= extent;
    }
    m_data.resize(values);
  }

  #pragma omp declare simd
  T &operator[](size_t index) {
    return m_data[index];
  }

  #pragma omp declare simd
  const T &operator[](size_t index) const {
    return m_data[index];
  }

  #pragma omp declare simd
  T &operator[](const std::array<size_t, D> &pos) {
    size_t index = 0;

    for (size_t i = 1; i <= D; i++) {
      index *= m_size[D - i];
      index += pos[D - i];
    }

    return m_data[index];
  }

  #pragma omp declare simd
  const T &operator[](const std::array<size_t, D> &pos) const {
    size_t index = 0;

    for (size_t i = 1; i <= D; i++) {
      index *= m_size[D - i];
      index += pos[D - i];
    }

    return m_data[index];
  }

  #pragma omp declare simd
  const std::array<size_t, D> &size() const {
    return m_size;
  }

  #pragma omp declare simd
  size_t stride(size_t depth = D) const {
    return depth ? m_size[depth - 1] * stride(depth - 1) : 1;
  }

  #pragma omp declare simd
  size_t size(size_t i) const {
    return m_size[i];
  }

  #pragma omp declare simd
  void fill(T value) {
    std::fill(m_data.begin(), m_data.end(), value);
  }

protected:
  static void *operator new(size_t);
  static void *operator new[](size_t);
};

template<size_t D>
struct moveBlock {
  template <typename IF, typename OF>
  moveBlock(
      IF &&input, const std::array<size_t, D> &input_size, const std::array<size_t, D> &input_offset,
      OF &&output, const std::array<size_t, D> &output_size, const std::array<size_t, D> &output_offset,
      const std::array<size_t, D> &size) {

    std::array<size_t, D - 1> input_subsize    {};
    std::array<size_t, D - 1> input_suboffset  {};
    std::array<size_t, D - 1> output_subsize   {};
    std::array<size_t, D - 1> output_suboffset {};
    std::array<size_t, D - 1> subsize          {};


    for (size_t i = 0; i < D - 1; i++) {
      input_subsize[i]    = input_size[i];
      input_suboffset[i]  = input_offset[i];
      output_subsize[i]   = output_size[i];
      output_suboffset[i] = output_offset[i];
      subsize[i]          = size[i];
    }

    size_t input_pos = input_offset[D - 1];
    size_t output_pos = output_offset[D - 1];

    const size_t input_end = std::min(input_offset[D - 1] + size[D - 1], input_size[D - 1]);
    const size_t output_end = std::min(output_offset[D - 1] + size[D - 1], output_size[D - 1]);

    std::array<size_t, D> full_input_pos {};
    std::array<size_t, D> full_output_pos {};

    while (input_pos < input_end && output_pos < output_end) {
      full_input_pos[D - 1] = input_pos;
      full_output_pos[D - 1] = output_pos;

      auto inputF = [&](const std::array<size_t, D - 1> &pos) {
        std::copy(std::begin(pos), std::end(pos), std::begin(full_input_pos));
        return input(full_input_pos);
      };

      auto outputF = [&](const std::array<size_t, D - 1> &pos, const auto &value) {
        std::copy(std::begin(pos), std::end(pos), std::begin(full_output_pos));
        output(full_output_pos, value);
      };

      moveBlock<D - 1>(inputF, input_subsize, input_suboffset, outputF, output_subsize, output_suboffset, subsize);

      input_pos++;
      output_pos++;
    }

    while (output_pos < output_end) {
      full_output_pos[D - 1] = output_pos;

      auto inputF = [&](const std::array<size_t, D - 1> &pos) {
        std::copy(std::begin(pos), std::end(pos), std::begin(full_input_pos));
        return input(full_input_pos);
      };

      auto outputF = [&](const std::array<size_t, D - 1> &pos, const auto &value) {
        std::copy(std::begin(pos), std::end(pos), std::begin(full_output_pos));
        output(full_output_pos, value);
      };

      moveBlock<D - 1>(inputF, input_subsize, input_suboffset, outputF, output_subsize, output_suboffset, subsize);

      output_pos++;
    }
  }
};

template<>
struct moveBlock<1> {

  /**
   * @brief The parital specialization for getting one sample.
   * @see getBlock<BS, D>::getBlock
   */
  template <typename IF, typename OF>
  moveBlock(
      IF &&input, const std::array<size_t, 1> &input_size, const std::array<size_t, 1> &input_offset,
      OF &&output, const std::array<size_t, 1> &output_size, const std::array<size_t, 1> &output_offset,
      const std::array<size_t, 1> &size) {
    size_t input_pos = input_offset[0];
    size_t output_pos = output_offset[0];

    const size_t input_end = std::min(input_offset[0] + size[0], input_size[0]);
    const size_t output_end = std::min(output_offset[0] + size[0], output_size[0]);

    std::array<size_t, 1> full_input_pos {};
    std::array<size_t, 1> full_output_pos {};

    while (input_pos < input_end && output_pos < output_end) {
      full_input_pos[0]  = input_pos;
      full_output_pos[0] = output_pos;

      output(full_output_pos, input(full_input_pos));
      input_pos++;
      output_pos++;
    }

    while (output_pos < output_end) {
      full_output_pos[0] = output_pos;

      output(full_output_pos, input(full_input_pos));
      output_pos++;
    }
  }
};
