#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

template<typename Source>
class IBitstream {
public:
  explicit IBitstream(Source source): m_source(std::move(source)) {}

  std::vector<bool> read(size_t size) {
    std::vector<bool> data(size);

    for (size_t i = 0; i < size; ++i) {
      data[i] = readBit();
    }

    return data;
  }

  bool readBit() {
    if (m_index >= 8) {
      m_accumulator = m_source();
      m_index = 0;
    }

    return (m_accumulator >> m_index++) & 1;
  }

private:
  uint8_t m_index {8};
  uint8_t m_accumulator {};
  Source m_source;
};

template<typename Sink>
class OBitstream {
public:
  explicit OBitstream(Sink sink): m_sink(std::move(sink)) {}

  void write(const std::vector<bool> &data) {
    for (bool bit : data) {
      writeBit(bit);
    }
  }

  void writeBit(bool bit) {
    m_accumulator |= static_cast<uint8_t>(bit) << m_index++;

    if (m_index >= 8) {
      flush();
    }
  }

  void flush() {
    if (m_index > 0) {
      m_sink(m_accumulator);
      m_index = 0;
      m_accumulator = 0;
    }
  }

private:
  uint8_t m_index {};
  uint8_t m_accumulator {};
  Sink m_sink;
};
