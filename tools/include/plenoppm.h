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
#include <vector>
#include <string>

inline int create_directory(const char *file_name) {
  size_t last_slash_pos = std::string(file_name).find_last_of('/');
  if (last_slash_pos != std::string::npos) {
    std::string command = "mkdir -p '" + std::string(file_name).substr(0, last_slash_pos) + "'";
    return system(command.c_str());
  }
  return 0;
}

int mapPPMs(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, std::vector<PPM> &data);
int createPPMs(const char *output_file_mask, uint64_t width, uint64_t height, uint32_t color_depth, std::vector<PPM> &data);
int loadPPMGrid(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, uint64_t &image_count, std::vector<uint8_t> &data);
