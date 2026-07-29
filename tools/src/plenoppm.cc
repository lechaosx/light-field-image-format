/******************************************************************************\
* SOUBOR: plenoppm.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"
#include "file_mask.h"

#include <stdexcept>
#include <system_error>
#include <utility>

std::vector<PPM> mapPPMs(std::string_view input_file_mask) {
  const FileMask file_name {input_file_mask};
  const size_t file_name_count = file_name.count();
  if (file_name_count == 0) {
    throw std::length_error("input file mask is too large");
  }

  std::vector<PPM> images;
  for (size_t image = 0; image < file_name_count; ++image) {
    try {
      PPM ppm = PPM::map(file_name[image]);
      if (!images.empty()
          && (ppm.width() != images.front().width()
              || ppm.height() != images.front().height()
              || ppm.color_depth() != images.front().color_depth())) {
        throw std::runtime_error("PPM dimensions or maxvals do not match");
      }
      images.push_back(std::move(ppm));
    } catch (const std::system_error &) {
      continue;
    }
  }

  if (images.empty()) {
    throw std::runtime_error("no PPM image loaded");
  }
  return images;
}
