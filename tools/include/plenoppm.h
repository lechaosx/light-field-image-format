#pragma once

#include <ppm.h>

#include <cstdint>
#include <filesystem>
#include <system_error>
#include <vector>
#include <string>

inline int create_directory(const std::filesystem::path &file_name) {
  if (!file_name.has_parent_path()) return 0;
  std::error_code ec;
  std::filesystem::create_directories(file_name.parent_path(), ec);
  return ec.value();
}

int mapPPMs(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, std::vector<PPM> &data);
int createPPMs(const char *output_file_mask, uint64_t width, uint64_t height, uint32_t color_depth, std::vector<PPM> &data);
