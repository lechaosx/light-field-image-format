#pragma once

#include "format.h"

#include <array>
#include <cstdint>
#include <iosfwd>
#include <span>
#include <vector>

namespace lfif {

using Pixel = std::array<uint16_t, 3>;

struct DecodedImage {
  Header header;
  std::vector<Pixel> pixels;
};

Header writeImage(std::ostream &output, Header header, std::span<const Pixel> pixels);
DecodedImage readImage(std::istream &input);

}
