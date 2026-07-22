#include <ppm.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

int main(int argc, char *argv[]) {
  const bool within_one = argc == 4 && std::string_view(argv[3]) == "--within-one";
  if (argc != 3 && !within_one) {
    return 1;
  }

  PPM expected;
  PPM actual;
  if (expected.mmapPPM(argv[1]) < 0 || actual.mmapPPM(argv[2]) < 0
      || expected.width() != actual.width()
      || expected.height() != actual.height()
      || expected.color_depth() != actual.color_depth()) {
    return 1;
  }

  const size_t pixel_count = static_cast<size_t>(expected.width() * expected.height());
  for (size_t pixel = 0; pixel < pixel_count; ++pixel) {
    const auto expected_pixel = expected.get(pixel);
    const auto actual_pixel = actual.get(pixel);
    for (size_t channel = 0; channel < expected_pixel.size(); ++channel) {
      const uint16_t difference = expected_pixel[channel] > actual_pixel[channel]
          ? expected_pixel[channel] - actual_pixel[channel]
          : actual_pixel[channel] - expected_pixel[channel];
      if (difference > static_cast<uint16_t>(within_one)) {
        return 1;
      }
    }
  }
  return 0;
}
