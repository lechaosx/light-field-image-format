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
    std::array<uint16_t, 3> result {};
    const std::span<const uint8_t> data = pixels();
    if (m_color_depth > 255) {
      const size_t offset = index * 6;
      for (size_t channel = 0; channel < result.size(); ++channel) {
        const size_t sample = offset + channel * 2;
        result[channel] =
            static_cast<uint16_t>(data[sample]) << 8 | data[sample + 1];
      }
    } else {
      const size_t offset = index * 3;
      for (size_t channel = 0; channel < result.size(); ++channel) {
        result[channel] = data[offset + channel];
      }
    }
    return result;
  }

  void put(size_t index, const std::array<uint16_t, 3> &value) {
    assert(m_writable);
    uint8_t *data = static_cast<uint8_t *>(m_file) + m_header_offset;
    if (m_color_depth > 255) {
      const size_t offset = index * 6;
      for (size_t channel = 0; channel < value.size(); ++channel) {
        const size_t sample = offset + channel * 2;
        data[sample] = static_cast<uint8_t>(value[channel] >> 8);
        data[sample + 1] = static_cast<uint8_t>(value[channel]);
      }
    } else {
      const size_t offset = index * 3;
      for (size_t channel = 0; channel < value.size(); ++channel) {
        data[offset + channel] = static_cast<uint8_t>(value[channel]);
      }
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
