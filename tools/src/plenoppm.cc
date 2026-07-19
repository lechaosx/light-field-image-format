/******************************************************************************\
* SOUBOR: plenoppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <print>

#include <plenoppm.h>
#include <file_mask.h>

int mapPPMs(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, std::vector<PPM> &data) {
  FileMask file_name = make_file_mask(input_file_mask);

  width       = 0;
  height      = 0;
  color_depth = 0;

  for (size_t image = 0; image < file_mask_count(file_name); image++) {
    PPM ppm {};

    if (ppm.mmapPPM(file_mask_name(file_name, image).c_str()) < 0) {
      continue;
    }

    if (width && height && color_depth) {
      if ((ppm.width() != width) || (ppm.height() != height) || (ppm.color_depth() != color_depth)) {
        std::println(stderr, "ERROR: PPMs DIMENSIONS MISMATCH");
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
  FileMask file_name = make_file_mask(output_file_mask);

  for (size_t image {}; image < data.size(); image++) {
    std::string name = file_mask_name(file_name, image);
    if (create_directory(name)) {
      return -1;
    }

    if (data[image].createPPM(name.c_str(), width, height, color_depth) < 0) {
      return -2;
    }
  }
  return 0;
}
