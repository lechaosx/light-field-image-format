#include "plenoppm.h"

#include <algorithm>
#include <glob.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

std::vector<PPM> mapPPMs(std::string_view input_file_mask) {
  std::string pattern;
  bool is_mask {};
  for (const char character : input_file_mask) {
    if (character == '#') {
      pattern += "[0-9]";
      is_mask = true;
    } else {
      if (character == '\\' || character == '*' || character == '?'
          || character == '[' || character == ']') {
        pattern += '\\';
      }
      pattern += character;
    }
  }

  std::vector<std::string> file_names;
  if (is_mask) {
    glob_t matches {};
    std::unique_ptr<glob_t, decltype(&globfree)> release_matches {
        &matches, &globfree};
    const int status = glob(pattern.c_str(), 0, nullptr, &matches);
    if (status != 0 && status != GLOB_NOMATCH) {
      throw std::runtime_error("cannot expand PPM file mask");
    }
    for (size_t i = 0; i < matches.gl_pathc; ++i) {
      file_names.emplace_back(matches.gl_pathv[i]);
    }
  } else {
    file_names.emplace_back(input_file_mask);
  }
  std::ranges::sort(file_names);

  std::vector<PPM> images;
  for (const std::string &file_name : file_names) {
    try {
      PPM ppm = PPM::map(file_name);
      if (!images.empty()
          && (ppm.width() != images.front().width()
              || ppm.height() != images.front().height()
              || ppm.color_depth() != images.front().color_depth())) {
        throw std::runtime_error("PPM dimensions or maxvals do not match");
      }
      images.push_back(std::move(ppm));
    } catch (const std::system_error &error) {
      if (error.code() == std::errc::no_such_file_or_directory) {
        continue;
      }
      throw;
    }
  }

  if (images.empty()) {
    throw std::runtime_error("no PPM image loaded");
  }
  return images;
}
