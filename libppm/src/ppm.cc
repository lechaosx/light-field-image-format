/******************************************************************************\
* SOUBOR: ppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "ppm.h"

#include <charconv>
#include <cinttypes>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>

using std::string;

namespace {

bool imageSize(uint64_t width, uint64_t height, uint32_t color_depth, size_t &size) {
  if (width == 0 || height == 0) {
    return false;
  }
  const size_t bytes_per_pixel = (color_depth > 255 ? 2U : 1U) * 3U;
  if (width > std::numeric_limits<size_t>::max() / height) {
    return false;
  }
  const size_t pixels = width * height;
  if (pixels > std::numeric_limits<size_t>::max() / bytes_per_pixel) {
    return false;
  }
  size = pixels * bytes_per_pixel;
  return true;
}

}

enum PPMHeaderParserState {
  STATE_INIT,
  STATE_P,
  STATE_P6,
  STATE_P6_SPACE,
  STATE_WIDTH,
  STATE_WIDTH_SPACE,
  STATE_HEIGHT,
  STATE_HEIGHT_SPACE,
  STATE_DEPTH,
  STATE_END
};

void skipUntilEol(FILE *input) {
  int c {};
  while((c = getc(input)) != EOF) {
    if (c == '\n') {
      return;
    }
  }
  return;
}

int PPM::parseHeader(FILE *ppm) {
  string str_width  {};
  string str_height {};
  string str_depth  {};

  PPMHeaderParserState state = STATE_INIT;

  int c {};
  while((c = getc(ppm)) != EOF) {
    switch (state) {
      case STATE_INIT:
      if (c == 'P') {
        state = STATE_P;
      }
      else {
        return -1;
      }
      break;

      case STATE_P:
      if (c == '6') {
        state = STATE_P6;
      }
      else {
        return -1;
      }
      break;

      case STATE_P6:
      if (c == '#') {
        skipUntilEol(ppm);
        state = STATE_P6_SPACE;
      }
      else if (isspace(c)) {
        state = STATE_P6_SPACE;
      }
      else {
        return -1;
      }
      break;

      case STATE_P6_SPACE:
      if (c == '#') {
        skipUntilEol(ppm);
      }
      else if (isspace(c)) {
        // STAY HERE
      }
      else if (isdigit(c)) {
        str_width += c;
        state = STATE_WIDTH;
      }
      else {
        return -1;
      }
      break;

      case STATE_WIDTH:
      if (c == '#') {
        skipUntilEol(ppm);
        state = STATE_WIDTH_SPACE;
      }
      else if (isspace(c)) {
        state = STATE_WIDTH_SPACE;
      }
      else if (isdigit(c)) {
        str_width += c;
      }
      else {
        return -1;
      }
      break;

      case STATE_WIDTH_SPACE:
      if (c == '#') {
        skipUntilEol(ppm);
      }
      else if (isspace(c)) {
        // STAY HERE
      }
      else if (isdigit(c)) {
        str_height += c;
        state = STATE_HEIGHT;
      }
      else {
        return -1;
      }
      break;

      case STATE_HEIGHT:
      if (c == '#') {
        skipUntilEol(ppm);
        state = STATE_HEIGHT_SPACE;
      }
      else if (isspace(c)) {
        state = STATE_HEIGHT_SPACE;
      }
      else if (isdigit(c)) {
        str_height += c;
      }
      else {
        return -1;
      }
      break;

      case STATE_HEIGHT_SPACE:
      if (c == '#') {
        skipUntilEol(ppm);
      }
      else if (isspace(c)) {
        // STAY HERE
      }
      else if (isdigit(c)) {
        str_depth += c;
        state = STATE_DEPTH;
      }
      else {
        return -1;
      }
      break;

      case STATE_DEPTH:
      if (c == '#') {
        skipUntilEol(ppm);
        state = STATE_END;
      }
      else if (isspace(c)) {
        state = STATE_END;
      }
      else if (isdigit(c)) {
        str_depth += c;
      }
      else {
        return -1;
      }
      break;

      case STATE_END:
      ungetc(c, ppm);

      auto parse = [](const std::string &text, uint64_t maximum, uint64_t &value) {
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        return result.ec == std::errc {} && result.ptr == text.data() + text.size() && value > 0 && value <= maximum;
      };

      uint64_t width {};
      if (!parse(str_width, std::numeric_limits<uint64_t>::max(), width)) {
        return -1;
      }

      uint64_t height {};
      if (!parse(str_height, std::numeric_limits<uint64_t>::max(), height)) {
        return -1;
      }

      uint64_t color_depth {};
      if (!parse(str_depth, 65535, color_depth)) {
        return -1;
      }

      m_width       = width;
      m_height      = height;
      m_color_depth = color_depth;

      return 0;
      break;
    }
  }

  return -1;
}

int PPM::mmapPPM(const char *file_name) {
  release();

  FILE *ppm = fopen(file_name, "rb");
  if (!ppm) {
    return -1;
  }

  if (parseHeader(ppm) < 0) {
    fclose(ppm);
    return -2;
  }

  const long header_offset = ftell(ppm);
  fclose(ppm);
  if (header_offset < 0) {
    return -3;
  }
  m_header_offset = header_offset;

  size_t pixel_data_size {};
  if (!imageSize(m_width, m_height, m_color_depth, pixel_data_size)
      || pixel_data_size > std::numeric_limits<size_t>::max() - m_header_offset) {
    return -3;
  }
  m_map_size = pixel_data_size + m_header_offset;

  int fd = open(file_name, O_RDONLY);
  if (fd < 0) {
    return -3;
  }

  struct stat file_stat {};
  if (fstat(fd, &file_stat) < 0 || file_stat.st_size < 0 || static_cast<uint64_t>(file_stat.st_size) < m_map_size) {
    close(fd);
    return -4;
  }

  m_file = mmap(NULL, m_map_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);

  if (m_file == MAP_FAILED) {
    m_file = nullptr;
    return -5;
  }

  m_opened = true;

  return 0;
}

PPM::PPM(PPM &&other) noexcept {
  *this = std::move(other);
}

PPM &PPM::operator=(PPM &&other) noexcept {
  if (this != &other) {
    release();
    m_opened = std::exchange(other.m_opened, false);
    m_width = std::exchange(other.m_width, 0);
    m_height = std::exchange(other.m_height, 0);
    m_color_depth = std::exchange(other.m_color_depth, 0);
    m_file = std::exchange(other.m_file, nullptr);
    m_map_size = std::exchange(other.m_map_size, 0);
    m_header_offset = std::exchange(other.m_header_offset, 0);
  }
  return *this;
}

PPM::~PPM() {
  release();
}

void PPM::release() {
  if (m_opened) {
    munmap(m_file, m_map_size);
  }
  m_opened = false;
  m_file = nullptr;
  m_map_size = 0;
}

int PPM::createPPM(const char *file_name, uint64_t width, uint64_t height, uint32_t color_depth) {
  release();

  if (width == 0 || height == 0 || color_depth == 0 || color_depth > 65535) {
    return -1;
  }

  size_t pixel_data_size {};
  if (!imageSize(width, height, color_depth, pixel_data_size)) {
    return -1;
  }

  FILE *ppm = fopen(file_name, "wb");
  if (!ppm) {
    return -1;
  }

  if (fprintf(ppm, "P6\n%" PRIu64 "\n%" PRIu64 "\n%u\n", width, height, color_depth) < 0) {
    fclose(ppm);
    return -1;
  }

  const long header_offset = ftell(ppm);
  const int close_result = fclose(ppm);
  if (header_offset < 0 || close_result != 0) {
    return -2;
  }
  m_header_offset = header_offset;
  if (pixel_data_size > std::numeric_limits<size_t>::max() - m_header_offset) {
    return -2;
  }
  m_map_size = pixel_data_size + m_header_offset;

  int fd = open(file_name, O_RDWR);
  if (fd < 0) {
    return -3;
  }

  if (ftruncate(fd, m_map_size) < 0) {
    close(fd);
    return -4;
  }

  m_file = mmap(NULL, m_map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  if (m_file == MAP_FAILED) {
    m_file = nullptr;
    return -5;
  }

  m_opened      = true;
  m_width       = width;
  m_height      = height;
  m_color_depth = color_depth;

  return 0;
}
