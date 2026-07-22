/******************************************************************************\
* SOUBOR: plenoppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"
#include "file_mask.h"

#include <cmath>
#include <iostream>
#include <limits>

using std::cerr;
using std::endl;

bool rgbDataSize(uint64_t width, uint64_t height, size_t image_count, size_t &size) {
  if (width == 0 || height == 0 || image_count == 0
      || width > static_cast<uint64_t>(std::numeric_limits<int>::max() / 3)
      || height > static_cast<uint64_t>(std::numeric_limits<int>::max())
      || width > std::numeric_limits<size_t>::max() / height) {
    return false;
  }
  const size_t pixels_per_image = static_cast<size_t>(width * height);
  if (image_count > std::numeric_limits<size_t>::max() / pixels_per_image) {
    return false;
  }
  const size_t grid_pixels = pixels_per_image * image_count;
  if (grid_pixels > std::numeric_limits<size_t>::max() / 3) {
    return false;
  }
  size = grid_pixels * 3;
  return true;
}

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

    size_t rgb_size {};
    if (!rgbDataSize(width, height, 1, rgb_size)) {
      cerr << "ERROR: PPM DIMENSIONS ARE TOO LARGE" << endl;
      return -1;
    }

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

  size_t data_size {};
  if (!rgbDataSize(width, height, images.size(), data_size)) {
    std::cerr << "ERROR: IMAGE GRID IS TOO LARGE\n";
    return -4;
  }

  const size_t pixels_per_image = data_size / images.size() / 3;
  data.resize(data_size);
  for (size_t image = 0; image < images.size(); ++image) {
    for (size_t pixel = 0; pixel < pixels_per_image; ++pixel) {
      const auto rgb = images[image].get(pixel);
      for (size_t component = 0; component < rgb.size(); ++component) {
        data[(image * pixels_per_image + pixel) * 3 + component] = rgb[component];
      }
    }
  }
  return 0;
}
