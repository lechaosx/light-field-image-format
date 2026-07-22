#pragma once

#include <algorithm>
#include <cstdint>

#include <endian.h>

template <typename T>
class BigEndian {
  T m_data;

  static T endianSwap(T data) {
    uint8_t *ptr = reinterpret_cast<uint8_t *>(&data);
    std::reverse(ptr, ptr + sizeof(T));
    return data;
  }

  static T bigEndianSwap(T data) {
    if (htobe16(1) == 1) {
      return data;
    }
    else {
      return endianSwap(data);
    }
  }

public:
  BigEndian() {}
  BigEndian(const T& other): m_data { bigEndianSwap(other) } { }
  operator T() const { return bigEndianSwap(m_data); }

};

template <typename T>
class LittleEndian {
  T m_data;

  static T endianSwap(T data) {
    uint8_t *ptr = reinterpret_cast<uint8_t *>(&data);
    std::reverse(ptr, ptr + sizeof(T));
    return data;
  }

  static T littleEndianSwap(T data) {
    if (htobe16(1) == 1) {
      return endianSwap(data);
    }
    else {
      return data;
    }
  }

  LittleEndian() { }
  LittleEndian(const T& other): m_data { littleEndianSwap(other) } { }
  operator T() const { return littleEndianSwap(m_data); }

private:
};
