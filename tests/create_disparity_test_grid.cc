#include <ppm.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace {

constexpr size_t extent = 8;

size_t wrap(int64_t coordinate) {
  coordinate %= static_cast<int64_t>(extent);
  if (coordinate < 0) {
    coordinate += extent;
  }
  return static_cast<size_t>(coordinate);
}

}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    throw std::invalid_argument("output prefix required");
  }

  for (size_t view_y {}; view_y < 2; ++view_y) {
    for (size_t view_x {}; view_x < 2; ++view_x) {
      const size_t view = view_y * 2 + view_x;
      PPM image = PPM::create(
          std::string(argv[1]) + "0" + std::to_string(view) + ".ppm",
          extent,
          extent,
          255);
      const int64_t horizontal_shift =
          (static_cast<int64_t>(view_x) - 1);
      const int64_t vertical_shift =
          -(static_cast<int64_t>(view_y) - 1);

      for (size_t y {}; y < extent; ++y) {
        for (size_t x {}; x < extent; ++x) {
          const size_t source_x =
              wrap(static_cast<int64_t>(x) - horizontal_shift);
          const size_t source_y =
              wrap(static_cast<int64_t>(y) - vertical_shift);
          const uint16_t value =
              static_cast<uint16_t>((source_x * 29 + source_y * 47
                                     + source_x * source_y * 11)
                                    % 251);
          image.put(
              y * extent + x,
              {value,
               static_cast<uint16_t>((value * 3 + 17) % 256),
               static_cast<uint16_t>((value * 7 + 31) % 256)});
        }
      }
      image.flush();
    }
  }
}
