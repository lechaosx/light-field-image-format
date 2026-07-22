#include <algorithm>
#include <format>
#include <string_view>

#include <file_mask.h>

FileMask make_file_mask(std::string_view input_file_mask) {
  return {std::string(input_file_mask)};
}

std::string file_mask_name(const FileMask &m, size_t index) {
  const size_t width = std::ranges::count(m.pattern, '#');
  const std::string image_number = std::format("{:0{}}", index, width);
  std::string file_name = m.pattern;
  size_t digit {};
  for (char &c : file_name) {
    if (c == '#') c = image_number[digit++];
  }

  return file_name;
}

size_t file_mask_count(const FileMask &m) {
  size_t result = 1;
  for (const char c : m.pattern) {
    if (c == '#') result *= 10;
  }
  return result;
}
