#pragma once

#include <array>

#include <cstddef>
#include <cstdio>
#include <cstdint>

#include "ppm_endian.h"

class PPM {
  bool        m_opened;

  uint64_t    m_width;
  uint64_t    m_height;
  uint32_t    m_color_depth;

  void       *m_file;
  size_t      m_header_offset;

  int parseHeader(FILE *ppm);

public:
  PPM()                       = default;
  PPM(const PPM &)            = delete;
  PPM &operator=(const PPM &) = delete;

  PPM(PPM &&);
  ~PPM();

  int createPPM(const char *file_name, uint64_t width, uint64_t height, uint32_t color_depth);

  int mmapPPM(const char *file_name);

  void *data() const {
    return static_cast<uint8_t *>(m_file) + m_header_offset;
  }

  std::array<uint16_t, 3> get(size_t index) const {
    uint16_t R {};
    uint16_t G {};
    uint16_t B {};

    if (m_color_depth > 255) {
      const BigEndian<uint16_t> *ptr = static_cast<const BigEndian<uint16_t> *>(data());
      R = ptr[index * 3 + 0];
      G = ptr[index * 3 + 1];
      B = ptr[index * 3 + 2];
    }
    else {
      const BigEndian<uint8_t> *ptr = static_cast<const BigEndian<uint8_t> *>(data());
      R = ptr[index * 3 + 0];
      G = ptr[index * 3 + 1];
      B = ptr[index * 3 + 2];
    }

    return {R, G, B};
  }

  void put(size_t index, const std::array<uint16_t, 3> &value) {
    if (m_color_depth > 255) {
      BigEndian<uint16_t> *ptr = static_cast<BigEndian<uint16_t> *>(data());
      ptr[index * 3 + 0] = value[0];
      ptr[index * 3 + 1] = value[1];
      ptr[index * 3 + 2] = value[2];
    }
    else {
      BigEndian<uint8_t> *ptr = static_cast<BigEndian<uint8_t> *>(data());
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
