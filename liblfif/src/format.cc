#include <lfif/format.h>

#include <array>
#include <bit>
#include <cstddef>
#include <istream>
#include <limits>
#include <stdexcept>
#include <string>

namespace lfif {
namespace {

constexpr std::array<uint8_t, 8> magic {0x4c, 0x46, 0x49, 0x46, 0x0d, 0x0a, 0x1a, 0x0a};
constexpr uint8_t current_major_version = 1;
constexpr uint8_t current_minor_version = 0;
constexpr size_t fixed_header_size = 48;
constexpr uint32_t prediction_flag = 1U << 0;
constexpr uint32_t disparity_flag = 1U << 1;
constexpr uint32_t known_flags = prediction_flag | disparity_flag;

void append(std::vector<uint8_t> &output, uint64_t value, size_t bytes) {
  for (size_t i = bytes; i > 0; --i) {
    output.push_back(static_cast<uint8_t>(value >> ((i - 1) * 8)));
  }
}

uint64_t read(std::istream &input, size_t bytes) {
  uint64_t value = 0;
  for (size_t i = 0; i < bytes; ++i) {
    const int byte = input.get();
    if (byte == std::char_traits<char>::eof()) {
      throw std::runtime_error("truncated LFIF header");
    }
    value = (value << 8) | static_cast<uint8_t>(byte);
  }
  return value;
}

void validate(const Header &header) {
  if (header.extents.empty() || header.extents.size() > std::numeric_limits<uint8_t>::max()) {
    throw std::invalid_argument("dimension count must be between 1 and 255");
  }
  if (header.block_extents.size() != header.extents.size()) {
    throw std::invalid_argument("image and block dimension counts must match");
  }
  if (header.channels != 3) {
    throw std::invalid_argument("LFIF version 1 requires three channels");
  }
  if (header.sample_depth == 0 || header.sample_depth > 16) {
    throw std::invalid_argument("sample depth must be between 1 and 16 bits");
  }
  if (header.sample_format != SampleFormat::unsigned_integer
      || (header.transform != Transform::wavelet && header.transform != Transform::dct)
      || header.entropy_codec != EntropyCodec::cabac
      || header.color_space != ColorSpace::rgb) {
    throw std::invalid_argument("unsupported LFIF format identifier");
  }
  if (header.discarded_bits > header.sample_depth) {
    throw std::invalid_argument("discarded bits exceed sample depth");
  }
  if (!header.disparity_compensated
      && (header.disparity_shift[0] != 0 || header.disparity_shift[1] != 0)) {
    throw std::invalid_argument("disparity shifts require disparity compensation");
  }

  size_t image_values = 1;
  size_t block_values = 1;
  size_t aligned_values = 1;
  for (size_t i = 0; i < header.extents.size(); ++i) {
    const uint64_t extent64 = header.extents[i];
    const uint64_t block_extent64 = header.block_extents[i];
    if (extent64 == 0 || block_extent64 == 0) {
      throw std::invalid_argument("image and block extents must be nonzero");
    }
    if (extent64 > std::numeric_limits<size_t>::max()
        || block_extent64 > std::numeric_limits<size_t>::max()) {
      throw std::length_error("LFIF extents do not fit size_t");
    }

    const size_t extent = static_cast<size_t>(extent64);
    const size_t block_extent = static_cast<size_t>(block_extent64);
    const size_t blocks = extent / block_extent + (extent % block_extent != 0);
    if (blocks > std::numeric_limits<size_t>::max() / block_extent) {
      throw std::length_error("aligned LFIF extents overflow");
    }
    const size_t aligned_extent = blocks * block_extent;
    if (image_values > std::numeric_limits<size_t>::max() / extent
        || block_values > std::numeric_limits<size_t>::max() / block_extent
        || aligned_values > std::numeric_limits<size_t>::max() / aligned_extent) {
      throw std::length_error("LFIF dimension products overflow");
    }
    image_values *= extent;
    block_values *= block_extent;
    aligned_values *= aligned_extent;
  }
}

} // namespace

std::vector<uint8_t> serializeHeader(const Header &header) {
  validate(header);

  const size_t header_size = fixed_header_size + 16 * header.extents.size();
  std::vector<uint8_t> output;
  output.reserve(header_size);
  output.insert(output.end(), magic.begin(), magic.end());
  append(output, current_major_version, 1);
  append(output, current_minor_version, 1);
  append(output, header_size, 2);

  uint32_t flags = 0;
  if (header.prediction) {
    flags |= prediction_flag;
  }
  if (header.disparity_compensated) {
    flags |= disparity_flag;
  }
  append(output, flags, 4);
  append(output, header.extents.size(), 1);
  append(output, header.channels, 1);
  append(output, header.sample_depth, 1);
  append(output, static_cast<uint8_t>(header.sample_format), 1);
  append(output, static_cast<uint8_t>(header.transform), 1);
  append(output, static_cast<uint8_t>(header.entropy_codec), 1);
  append(output, static_cast<uint8_t>(header.color_space), 1);
  append(output, header.discarded_bits, 1);
  append(output, header.payload_size, 8);
  append(output, std::bit_cast<uint64_t>(header.disparity_shift[0]), 8);
  append(output, std::bit_cast<uint64_t>(header.disparity_shift[1]), 8);
  for (const uint64_t extent : header.extents) {
    append(output, extent, 8);
  }
  for (const uint64_t extent : header.block_extents) {
    append(output, extent, 8);
  }
  return output;
}

Header parseHeader(std::istream &input) {
  for (const uint8_t expected : magic) {
    if (read(input, 1) != expected) {
      throw std::runtime_error("invalid LFIF magic");
    }
  }

  const uint8_t major_version = read(input, 1);
  read(input, 1);
  if (major_version != current_major_version) {
    throw std::runtime_error("unsupported LFIF major version");
  }

  const size_t header_size = read(input, 2);
  const uint32_t flags = read(input, 4);
  if ((flags & ~known_flags) != 0) {
    throw std::runtime_error("unsupported LFIF flags");
  }

  Header header;
  const size_t dimensions = read(input, 1);
  header.channels = read(input, 1);
  header.sample_depth = read(input, 1);
  header.sample_format = static_cast<SampleFormat>(read(input, 1));
  header.transform = static_cast<Transform>(read(input, 1));
  header.entropy_codec = static_cast<EntropyCodec>(read(input, 1));
  header.color_space = static_cast<ColorSpace>(read(input, 1));
  header.discarded_bits = read(input, 1);
  header.payload_size = read(input, 8);
  header.disparity_shift[0] = std::bit_cast<int64_t>(read(input, 8));
  header.disparity_shift[1] = std::bit_cast<int64_t>(read(input, 8));
  header.prediction = (flags & prediction_flag) != 0;
  header.disparity_compensated = (flags & disparity_flag) != 0;

  const size_t required_size = fixed_header_size + 16 * dimensions;
  if (header_size < required_size) {
    throw std::runtime_error("LFIF header size is too small");
  }
  header.extents.resize(dimensions);
  header.block_extents.resize(dimensions);
  for (uint64_t &extent : header.extents) {
    extent = read(input, 8);
  }
  for (uint64_t &extent : header.block_extents) {
    extent = read(input, 8);
  }
  for (size_t i = required_size; i < header_size; ++i) {
    read(input, 1);
  }

  try {
    validate(header);
  } catch (const std::exception &error) {
    throw std::runtime_error(std::string("invalid LFIF header: ") + error.what());
  }
  return header;
}

} // namespace lfif
