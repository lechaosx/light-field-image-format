/**
* @file bitstream.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 12. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Stream wrapper classes for streaming bits.
*/

#pragma once

#include <cstdint>

#include <istream>
#include <ostream>
#include <vector>

class IBitstream {
  std::istream *m_stream {};
  uint8_t m_index {};
  uint8_t m_accumulator {};

public:
  IBitstream() = default;

  IBitstream(std::istream &stream): m_stream { &stream } { m_index = 8; }

  void open(std::istream &stream) { m_stream = &stream; m_index = 8; }

  [[nodiscard]] std::vector<bool> read(const size_t size) {
    std::vector<bool> data(size);
    for (size_t i = 0; i < size; i++) {
      data[i] = readBit();
    }
    return data;
  }

  [[nodiscard]] bool readBit() {
    if (m_index >= 8) {
      m_accumulator = m_stream->get();
      m_index = 0;
    }
    bool data = (m_accumulator >> m_index) & 1;
    m_index++;
    return data;
  }

  [[nodiscard]] bool eof() { return (m_index >= 8) && m_stream->eof(); }
};

class OBitstream {
  std::ostream *m_stream {};
  uint8_t m_index {};
  uint8_t m_accumulator {};

public:
  OBitstream() = default;

  OBitstream(std::ostream &stream): m_stream { &stream } {}

  ~OBitstream() { flush(); }

  void open(std::ostream &stream) { m_stream = &stream; }

  void write(const std::vector<bool> &data) {
    for (auto &&bit: data) {
      writeBit(bit);
    }
  }

  void writeBit(const bool bit) {
    m_accumulator |= bit << m_index;
    m_index++;
    if (m_index >= 8) {
      flush();
    }
  }

  void flush() {
    if (m_index > 0) {
      m_stream->put(m_accumulator);
      m_index = 0;
      m_accumulator = 0;
    }
  }
};
