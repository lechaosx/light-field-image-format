#pragma once

#include <bit>
#include <concepts>
#include <istream>
#include <ostream>

template <std::integral T>
inline T bigEndianSwap(T data) {
  return std::endian::native == std::endian::big ? data : std::byteswap(data);
}

template <std::integral T>
inline T littleEndianSwap(T data) {
  return std::endian::native == std::endian::little ? data : std::byteswap(data);
}

template<std::integral T>
inline void writeValueToStream(T data, std::ostream &stream) {
  T dataBE = bigEndianSwap(data);
  stream.write(reinterpret_cast<const char *>(&dataBE), sizeof(dataBE));
}

template<std::integral T>
inline T readValueFromStream(std::istream &stream) {
  T dataBE {};
  stream.read(reinterpret_cast<char *>(&dataBE), sizeof(dataBE));
  return bigEndianSwap(dataBE);
}
