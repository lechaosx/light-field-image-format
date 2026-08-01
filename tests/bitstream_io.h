#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

inline auto byteSource(std::istream &input) {
  return [&input]() -> uint8_t {
    const auto value = input.get();
    if (value == std::istream::traits_type::eof()) {
      throw std::ios_base::failure(
          input.eof()
              ? "unexpected end of bitstream"
              : "failed to read bitstream");
    }
    return static_cast<uint8_t>(value);
  };
}

inline auto byteSink(std::ostream &output) {
  return [&output](uint8_t byte) {
    output.put(static_cast<char>(byte));
    if (!output) {
      throw std::ios_base::failure("failed to write bitstream");
    }
  };
}
