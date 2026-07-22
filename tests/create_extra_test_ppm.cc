#include <ppm.h>

#include <array>
#include <cstdint>
#include <string_view>

int main(int argc, char *argv[]) {
  if (argc == 3 && std::string_view(argv[1]) == "--check") {
    PPM image;
    return image.mmapPPM(argv[2]) < 0 ? 1 : 0;
  }
  if (argc != 2) {
    return 1;
  }

  PPM image;
  if (image.createPPM(argv[1], 64, 64, 255) < 0) {
    return 1;
  }
  for (size_t pixel = 0; pixel < 64 * 64; ++pixel) {
    image.put(pixel, std::array<uint16_t, 3> {
        static_cast<uint16_t>((pixel * 17) % 256),
        static_cast<uint16_t>((pixel * 31 + 7) % 256),
        static_cast<uint16_t>((pixel * 47 + 3) % 256),
    });
  }
  return 0;
}
