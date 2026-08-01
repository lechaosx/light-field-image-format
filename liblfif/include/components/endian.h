#pragma once

#include <bit>
#include <concepts>
#include <istream>
#include <ostream>

template<std::integral T>
constexpr T bigEndianSwap(T data) {
  if constexpr (std::endian::native == std::endian::big) {
    return data;
  }
  else {
    return std::byteswap(data);
  }
}

template<std::integral T>
constexpr T littleEndianSwap(T data) {
  if constexpr (std::endian::native == std::endian::little) {
    return data;
  }
  else {
    return std::byteswap(data);
  }
}

template<std::integral T>
void writeValueToStream(T data, std::ostream &stream) {
  const T big_endian_data = bigEndianSwap(data);
  stream.write(
      reinterpret_cast<const char *>(&big_endian_data),
      sizeof(big_endian_data));
  if (!stream) {
    throw std::ios_base::failure("failed to write fixed-width value");
  }
}

template<std::integral T>
T readValueFromStream(std::istream &stream) {
  T big_endian_data {};
  stream.read(
      reinterpret_cast<char *>(&big_endian_data),
      sizeof(big_endian_data));
  if (!stream) {
    throw std::ios_base::failure("truncated fixed-width value");
  }
  return bigEndianSwap(big_endian_data);
}
