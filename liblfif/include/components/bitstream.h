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

struct IBitstream {
  std::istream *stream    {};
  uint8_t       index     {};
  uint8_t       accumulator {};
};

inline void open(IBitstream &bs, std::istream &stream) {
  bs.stream = &stream;
  bs.index  = 8;
}

[[nodiscard]] inline bool readBit(IBitstream &bs) {
  if (bs.index >= 8) {
    bs.accumulator = bs.stream->get();
    bs.index = 0;
  }
  return (bs.accumulator >> bs.index++) & 1;
}

[[nodiscard]] inline bool eof(const IBitstream &bs) {
  return (bs.index >= 8) && bs.stream->eof();
}

class OBitstream {
  std::ostream *m_stream     {};
  uint8_t       m_index      {};
  uint8_t       m_accumulator {};

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

  void writeBit(bool bit) {
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
