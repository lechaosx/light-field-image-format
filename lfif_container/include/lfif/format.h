#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <vector>

namespace lfif {

enum class SampleFormat : uint8_t {
  unsigned_integer = 1,
};

enum class Transform : uint8_t {
  wavelet = 1,
  dct = 2,
};

enum class EntropyCodec : uint8_t {
  cabac = 1,
};

enum class ColorSpace : uint8_t {
  rgb = 1,
};

struct Header {
  std::vector<uint64_t> extents;
  std::vector<uint64_t> block_extents;
  uint8_t sample_depth {8};
  uint8_t channels {3};
  SampleFormat sample_format {SampleFormat::unsigned_integer};
  Transform transform {Transform::wavelet};
  EntropyCodec entropy_codec {EntropyCodec::cabac};
  ColorSpace color_space {ColorSpace::rgb};
  uint8_t discarded_bits {};
  bool prediction {};
  bool disparity_compensated {};
  std::array<int64_t, 2> disparity_shift {};
  uint64_t payload_size {};

  bool operator==(const Header &) const = default;
};

std::vector<uint8_t> serializeHeader(const Header &header);
Header parseHeader(std::istream &input);

}
