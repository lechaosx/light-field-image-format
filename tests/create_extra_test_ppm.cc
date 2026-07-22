#include <ppm.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

int createImage(const std::string &path, size_t seed) {
  PPM image;
  if (image.createPPM(path.c_str(), 64, 64, 255) < 0) {
    return 1;
  }
  for (size_t pixel = 0; pixel < 64 * 64; ++pixel) {
    image.put(pixel, std::array<uint16_t, 3> {
        static_cast<uint16_t>((pixel * 17 + seed) % 256),
        static_cast<uint16_t>((pixel * 31 + 7 + seed) % 256),
        static_cast<uint16_t>((pixel * 47 + 3 + seed) % 256),
    });
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc == 3 && std::string_view(argv[1]) == "--check") {
    PPM image;
    return image.mmapPPM(argv[2]) < 0 ? 1 : 0;
  }
  if (argc == 4 && std::string_view(argv[1]) == "--grid") {
    size_t count = std::stoul(argv[3]);
    for (size_t image = 0; image < count; ++image) {
      std::string index = std::to_string(image);
      if (index.size() < 2) {
        index.insert(index.begin(), 2 - index.size(), '0');
      }
      if (createImage(std::string(argv[2]) + index + ".ppm", image) != 0) {
        return 1;
      }
    }
    return 0;
  }
  if (argc != 2) {
    return 1;
  }

  return createImage(argv[1], 0);
}
