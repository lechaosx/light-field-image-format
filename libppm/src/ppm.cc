/******************************************************************************\
* SOUBOR: ppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "ppm.h"

#include <charconv>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace {

std::optional<size_t> pixelDataSize(
    uint64_t width, uint64_t height, uint32_t color_depth) {
  if (width == 0 || height == 0) {
    return std::nullopt;
  }
  const size_t bytes_per_pixel = (color_depth > 255 ? 2U : 1U) * 3U;
  if (width > std::numeric_limits<size_t>::max() / height) {
    return std::nullopt;
  }
  const size_t pixels = width * height;
  if (pixels > std::numeric_limits<size_t>::max() / bytes_per_pixel) {
    return std::nullopt;
  }
  return pixels * bytes_per_pixel;
}

struct ParsedHeader {
  uint64_t width;
  uint64_t height;
  uint32_t color_depth;
  size_t data_offset;
};

ParsedHeader parseHeader(std::span<const uint8_t> bytes) {
  size_t offset {};
  if (bytes.size() < 2 || bytes[0] != 'P' || bytes[1] != '6') {
    throw std::runtime_error("invalid PPM magic");
  }
  offset = 2;

  if (offset == bytes.size()
      || (!std::isspace(static_cast<unsigned char>(bytes[offset]))
          && bytes[offset] != '#')) {
    throw std::runtime_error("invalid PPM header");
  }

  auto readNumber = [&bytes, &offset](uint64_t maximum) {
    while (true) {
      while (offset < bytes.size()
             && std::isspace(static_cast<unsigned char>(bytes[offset]))) {
        ++offset;
      }
      if (offset < bytes.size() && bytes[offset] == '#') {
        while (offset < bytes.size() && bytes[offset] != '\n') {
          ++offset;
        }
        continue;
      }
      break;
    }

    const size_t start = offset;
    while (offset < bytes.size()
           && std::isdigit(static_cast<unsigned char>(bytes[offset]))) {
      if (offset - start == std::numeric_limits<uint64_t>::digits10 + 1) {
        throw std::runtime_error("PPM header number is too large");
      }
      ++offset;
    }

    if (start == offset || offset == bytes.size()) {
      throw std::runtime_error("invalid PPM header");
    }

    const std::string_view text(
        reinterpret_cast<const char *>(bytes.data() + start), offset - start);
    uint64_t value {};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc {} || result.ptr != text.data() + text.size()
        || value == 0 || value > maximum) {
      throw std::runtime_error("invalid PPM header number");
    }

    if (bytes[offset] == '#') {
      while (offset < bytes.size() && bytes[offset] != '\n') {
        ++offset;
      }
      if (offset == bytes.size()) {
        throw std::runtime_error("invalid PPM header");
      }
      ++offset;
    } else if (std::isspace(static_cast<unsigned char>(bytes[offset]))) {
      ++offset;
    } else {
      throw std::runtime_error("invalid PPM header");
    }

    return value;
  };

  const uint64_t width = readNumber(std::numeric_limits<uint64_t>::max());
  const uint64_t height = readNumber(std::numeric_limits<uint64_t>::max());
  const uint64_t color_depth = readNumber(65535);
  return {width, height, static_cast<uint32_t>(color_depth), offset};
}

}

PPM PPM::map(const std::filesystem::path &file_name) {
  const int descriptor = open(file_name.c_str(), O_RDONLY);
  if (descriptor < 0) {
    throw std::system_error(
        errno, std::generic_category(), "cannot open PPM " + file_name.string());
  }

  struct stat file_stat {};
  if (fstat(descriptor, &file_stat) < 0) {
    const int error = errno;
    close(descriptor);
    throw std::system_error(
        error, std::generic_category(), "cannot inspect PPM " + file_name.string());
  }
  if (file_stat.st_size <= 0
      || static_cast<uintmax_t>(file_stat.st_size)
          > std::numeric_limits<size_t>::max()) {
    close(descriptor);
    throw std::runtime_error("invalid PPM file size: " + file_name.string());
  }
  const size_t map_size = static_cast<size_t>(file_stat.st_size);

  void *mapping = mmap(nullptr, map_size, PROT_READ, MAP_PRIVATE, descriptor, 0);
  const int mapping_error = errno;
  if (close(descriptor) < 0) {
    const int close_error = errno;
    if (mapping != MAP_FAILED) {
      munmap(mapping, map_size);
    }
    throw std::system_error(
        close_error, std::generic_category(), "cannot close PPM " + file_name.string());
  }
  if (mapping == MAP_FAILED) {
    throw std::system_error(
        mapping_error, std::generic_category(), "cannot map PPM " + file_name.string());
  }

  ParsedHeader header {};
  try {
    header = parseHeader(std::span(static_cast<const uint8_t *>(mapping), map_size));
  } catch (...) {
    munmap(mapping, map_size);
    throw;
  }

  const std::optional<size_t> pixel_data_size =
      pixelDataSize(header.width, header.height, header.color_depth);
  if (!pixel_data_size
      || header.data_offset > map_size
      || *pixel_data_size > map_size - header.data_offset) {
    munmap(mapping, map_size);
    throw std::runtime_error("truncated or oversized PPM pixel data: " + file_name.string());
  }

  PPM ppm;
  ppm.m_file = mapping;
  ppm.m_map_size = map_size;
  ppm.m_header_offset = header.data_offset;
  ppm.m_data_size = *pixel_data_size;
  ppm.m_width = header.width;
  ppm.m_height = header.height;
  ppm.m_color_depth = header.color_depth;
  const size_t pixel_count =
      *pixel_data_size / ((ppm.m_color_depth > 255 ? 2U : 1U) * 3U);
  for (size_t pixel = 0; pixel < pixel_count; ++pixel) {
    const auto samples = ppm.get(pixel);
    if (samples[0] > ppm.m_color_depth || samples[1] > ppm.m_color_depth
        || samples[2] > ppm.m_color_depth) {
      throw std::runtime_error("PPM sample exceeds maxval: " + file_name.string());
    }
  }
  return ppm;
}

PPM::PPM(PPM &&other) noexcept {
  *this = std::move(other);
}

PPM &PPM::operator=(PPM &&other) noexcept {
  if (this != &other) {
    release();
    m_width = std::exchange(other.m_width, 0);
    m_height = std::exchange(other.m_height, 0);
    m_color_depth = std::exchange(other.m_color_depth, 0);
    m_file = std::exchange(other.m_file, nullptr);
    m_map_size = std::exchange(other.m_map_size, 0);
    m_header_offset = std::exchange(other.m_header_offset, 0);
    m_data_size = std::exchange(other.m_data_size, 0);
    m_writable = std::exchange(other.m_writable, false);
  }
  return *this;
}

PPM::~PPM() {
  release();
}

void PPM::release() {
  if (m_file) {
    munmap(m_file, m_map_size);
  }
  m_file = nullptr;
  m_map_size = 0;
  m_header_offset = 0;
  m_data_size = 0;
  m_writable = false;
}

PPM PPM::create(
    const std::filesystem::path &file_name,
    uint64_t width,
    uint64_t height,
    uint32_t color_depth) {
  if (width == 0 || height == 0 || color_depth == 0 || color_depth > 65535) {
    throw std::invalid_argument("invalid PPM dimensions or maxval");
  }

  const std::optional<size_t> pixel_data_size =
      pixelDataSize(width, height, color_depth);
  if (!pixel_data_size) {
    throw std::length_error("PPM dimensions are too large");
  }

  std::string header = "P6\n";
  header += std::to_string(width);
  header += '\n';
  header += std::to_string(height);
  header += '\n';
  header += std::to_string(color_depth);
  header += '\n';

  if (*pixel_data_size > std::numeric_limits<size_t>::max() - header.size()) {
    throw std::length_error("PPM file is too large");
  }
  const size_t map_size = *pixel_data_size + header.size();
  if (map_size > static_cast<uintmax_t>(std::numeric_limits<off_t>::max())) {
    throw std::length_error("PPM file is too large");
  }

  const int descriptor = open(file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (descriptor < 0) {
    throw std::system_error(
        errno, std::generic_category(), "cannot create PPM " + file_name.string());
  }
  if (ftruncate(descriptor, static_cast<off_t>(map_size)) < 0) {
    const int error = errno;
    close(descriptor);
    throw std::system_error(
        error, std::generic_category(), "cannot size PPM " + file_name.string());
  }

  void *mapping =
      mmap(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
  const int mapping_error = errno;
  if (close(descriptor) < 0) {
    const int close_error = errno;
    if (mapping != MAP_FAILED) {
      munmap(mapping, map_size);
    }
    throw std::system_error(
        close_error, std::generic_category(), "cannot close PPM " + file_name.string());
  }
  if (mapping == MAP_FAILED) {
    throw std::system_error(
        mapping_error, std::generic_category(), "cannot map PPM " + file_name.string());
  }

  std::memcpy(mapping, header.data(), header.size());
  PPM ppm;
  ppm.m_file = mapping;
  ppm.m_map_size = map_size;
  ppm.m_header_offset = header.size();
  ppm.m_data_size = *pixel_data_size;
  ppm.m_width = width;
  ppm.m_height = height;
  ppm.m_color_depth = color_depth;
  ppm.m_writable = true;
  return ppm;
}

void PPM::flush() {
  if (!m_file || !m_writable) {
    throw std::logic_error("cannot flush read-only or empty PPM");
  }
  if (msync(m_file, m_map_size, MS_SYNC) < 0) {
    throw std::system_error(errno, std::generic_category(), "cannot flush PPM");
  }
}
