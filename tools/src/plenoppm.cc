/******************************************************************************\
* SOUBOR: plenoppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"
#include "file_mask.h"

#include <cmath>
#include <iostream>

using std::cerr;
using std::endl;

int mapPPMs(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, std::vector<PPM> &data) {
  FileMask file_name(input_file_mask);
  const size_t file_name_count = file_name.count();
  if (file_name_count == 0) {
    cerr << "ERROR: INPUT FILE MASK IS TOO LARGE" << endl;
    return -1;
  }

  width       = 0;
  height      = 0;
  color_depth = 0;

  for (size_t image = 0; image < file_name_count; image++) {
    PPM ppm {};

    if (ppm.mmapPPM(file_name[image].c_str()) < 0) {
      continue;
    }

    if (width && height && color_depth) {
      if ((ppm.width() != width) || (ppm.height() != height) || (ppm.color_depth() != color_depth)) {
        cerr << "ERROR: PPMs DIMENSIONS MISMATCH" << endl;
        return -1;
      }
    }

    width       = ppm.width();
    height      = ppm.height();
    color_depth = ppm.color_depth();

    data.push_back(std::move(ppm));
  }
  return 0;
}

int createPPMs(const char *output_file_mask, uint64_t width, uint64_t height, uint32_t color_depth, std::vector<PPM> &data) {
  FileMask file_name(output_file_mask);
  const size_t file_name_count = file_name.count();
  if (file_name_count == 0 || data.size() > file_name_count) {
    cerr << "ERROR: OUTPUT FILE MASK CANNOT REPRESENT ALL IMAGES" << endl;
    return -3;
  }

  for (size_t image {}; image < data.size(); image++) {
    if (create_directory(file_name[image].c_str())) {
      return -1;
    }

    if (data[image].createPPM(file_name[image].c_str(), width, height, color_depth) < 0) {
      return -2;
    }
  }
  return 0;
}

int loadPPMGrid(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, uint64_t &image_count, std::vector<uint8_t> &data) {
  std::vector<PPM> images;
  if (mapPPMs(input_file_mask, width, height, color_depth, images) < 0 || images.empty()) {
    std::cerr << "ERROR: NO IMAGE LOADED\n";
    return -1;
  }

  image_count = images.size();
  const uint64_t side = std::sqrt(image_count);
  if (side * side != image_count) {
    std::cerr << "ERROR: NOT SQUARE\n";
    return -2;
  }
  if (color_depth > 255) {
    std::cerr << "ERROR: BENCHMARK SUPPORTS ONLY 8-BIT PPM INPUT\n";
    return -3;
  }

  data.resize(width * height * image_count * 3);
  for (size_t image = 0; image < images.size(); ++image) {
    for (size_t pixel = 0; pixel < width * height; ++pixel) {
      const auto rgb = images[image].get(pixel);
      for (size_t component = 0; component < rgb.size(); ++component) {
        data[(image * width * height + pixel) * 3 + component] = rgb[component];
      }
    }
  }
  return 0;
}
