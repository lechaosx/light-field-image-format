/******************************************************************************\
* SOUBOR: plenoppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <iostream>

#include <plenoppm.h>
#include <file_mask.h>

using namespace std;

int mapPPMs(const char *input_file_mask, uint64_t &width, uint64_t &height, uint32_t &color_depth, std::vector<PPM> &data) {
  FileMask file_name(input_file_mask);

  width       = 0;
  height      = 0;
  color_depth = 0;

  for (size_t image = 0; image < file_name.count(); image++) {
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
