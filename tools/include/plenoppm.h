/**
* @file plenoppm.h
* @author Drahomír Dlabaja (xdlaba02)
* @date 13. 5. 2019
* @copyright 2019 Drahomír Dlabaja
* @brief Module for reading and writin ppm plenoptic images.
*/

#pragma once

#include <ppm.h>

#include <cstdint>
#include <filesystem>
#include <vector>

inline int create_directory(const char *file_name) {
  const std::filesystem::path parent = std::filesystem::path(file_name).parent_path();
  if (parent.empty()) {
    return 0;
  }

  std::error_code error;
  std::filesystem::create_directories(parent, error);
  return error ? -1 : 0;
}

int mapPPMs(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, std::vector<PPM> &data);
int createPPMs(const char *output_file_mask, uint64_t width, uint64_t height, uint32_t color_depth, std::vector<PPM> &data);
int loadPPMGrid(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, uint64_t &image_count, std::vector<uint8_t> &data);
