#pragma once

#include <cstdint>

#include <istream>
#include <ostream>
#include <vector>

// Sub-byte bit packing state. The stream is passed to each method rather than
// stored, so a bitstream is just the in-flight byte plus a bit index.
class IBitstream {
  uint8_t m_index       {8}; // 8 forces a fresh byte read on the first bit
  uint8_t m_accumulator {};

public:
  [[nodiscard]] bool readBit(std::istream &stream) {
    if (m_index >= 8) {
      m_accumulator = stream.get();
      m_index = 0;
    }
    return (m_accumulator >> m_index++) & 1;
  }

  [[nodiscard]] bool eof(std::istream &stream) const {
    return (m_index >= 8) && stream.eof();
  }
};

class OBitstream {
  uint8_t m_index       {};
  uint8_t m_accumulator {};

public:
  // Flushing is explicit: ostream operations may throw, so the destructor
  // intentionally performs no I/O.
  void writeBit(std::ostream &stream, bool bit) {
    m_accumulator |= bit << m_index;
    m_index++;
    if (m_index >= 8) {
      flush(stream);
    }
  }

  void write(std::ostream &stream, const std::vector<bool> &data) {
    for (const bool bit: data) {
      writeBit(stream, bit);
    }
  }

  void flush(std::ostream &stream) {
    if (m_index > 0) {
      stream.put(m_accumulator);
      m_index = 0;
      m_accumulator = 0;
    }
  }
};
