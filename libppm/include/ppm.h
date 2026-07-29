/**
* @file ppm.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Tiny library for reading and writing ppm files.
*/

#ifndef PPM_H
#define PPM_H

#include <array>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

#include "ppm_endian.h"

class PPM {
  uint64_t    m_width {};       /**< @brief Image width in pixels.*/
  uint64_t    m_height {};      /**< @brief Image height in pixels.*/
  uint32_t    m_color_depth {}; /**< @brief Maximum RGB value of an image.*/

  void       *m_file {};
  size_t      m_map_size {};
  size_t      m_header_offset {};
  size_t      m_data_size {};
  bool        m_writable {};

  PPM() = default;
  void release();

public:
  PPM(const PPM &)            = delete;
  PPM &operator=(const PPM &) = delete;

  PPM(PPM &&) noexcept;
  PPM &operator=(PPM &&) noexcept;
  ~PPM();

  [[nodiscard]] static PPM create(
      const std::filesystem::path &file_name,
      uint64_t width,
      uint64_t height,
      uint32_t color_depth);

  [[nodiscard]] static PPM map(const std::filesystem::path &file_name);
  void flush();

  std::span<const uint8_t> pixels() const {
    return {
        static_cast<const uint8_t *>(m_file) + m_header_offset,
        m_data_size,
    };
  }

  std::array<uint16_t, 3> get(size_t index) const {
    uint16_t R {};
    uint16_t G {};
    uint16_t B {};

    if (m_color_depth > 255) {
      const BigEndian<uint16_t> *ptr =
          reinterpret_cast<const BigEndian<uint16_t> *>(pixels().data());
      R = ptr[index * 3 + 0];
      G = ptr[index * 3 + 1];
      B = ptr[index * 3 + 2];
    }
    else {
      const BigEndian<uint8_t> *ptr =
          reinterpret_cast<const BigEndian<uint8_t> *>(pixels().data());
      R = ptr[index * 3 + 0];
      G = ptr[index * 3 + 1];
      B = ptr[index * 3 + 2];
    }

    return {R, G, B};
  }

  void put(size_t index, const std::array<uint16_t, 3> &value) {
    assert(m_writable);
    if (m_color_depth > 255) {
      BigEndian<uint16_t> *ptr = reinterpret_cast<BigEndian<uint16_t> *>(
          static_cast<uint8_t *>(m_file) + m_header_offset);
      ptr[index * 3 + 0] = value[0];
      ptr[index * 3 + 1] = value[1];
      ptr[index * 3 + 2] = value[2];
    }
    else {
      BigEndian<uint8_t> *ptr = reinterpret_cast<BigEndian<uint8_t> *>(
          static_cast<uint8_t *>(m_file) + m_header_offset);
      ptr[index * 3 + 0] = value[0];
      ptr[index * 3 + 1] = value[1];
      ptr[index * 3 + 2] = value[2];
    }
  }

  uint64_t width() const {
    return m_width;
  }

  uint64_t height() const {
    return m_height;
  }

  uint32_t color_depth() const {
    return m_color_depth;
  }
};

#endif
