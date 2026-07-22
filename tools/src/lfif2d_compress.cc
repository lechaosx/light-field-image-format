#include <bit>
#include <charconv>
#include <fstream>
#include <print>

#include <lfwf_encoder.h>
#include <ppm.h>

#include <compress.h>
#include <file_format.h>
#include <plenoppm.h>

int main(int argc, char *argv[]) {
  const char *input_file_name  {};
  const char *output_file_name {};

  uint8_t distortion {};
  bool    predict    {};
  bool    do_shift   {};

  if (!parse_args(argc, argv, input_file_name, output_file_name, distortion, predict, do_shift)) {
    return 1;
  }

  PPM ppm_image {};

  if (ppm_image.mmapPPM(input_file_name) < 0) {
    std::println(stderr, "ERROR: CANNOT OPEN IMAGE");
    return 2;
  }

  uint8_t color_depth = std::bit_width(ppm_image.color_depth());

  std::array<uint64_t, 2> image_size { ppm_image.width(), ppm_image.height() };
  std::array<uint64_t, 2> block_size {8, 8};
  if (optind + 2 == argc) {
    for (size_t i = 0; i < 2; i++) {
      std::string_view arg { argv[optind] };
      size_t tmp {};
      auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), tmp);
      if (ec != std::errc{}) {
        block_size = {8, 8};
        break;
      }
      block_size[i] = tmp;
      optind++;
    }
  }

  auto puller = [&](const std::array<size_t, 2> &pos) -> std::array<uint16_t, 3> {
    return ppm_image.get(pos[1] * image_size[0] + pos[0]);
  };

  if (create_directory(output_file_name)) {
    std::println(stderr, "ERROR: CANNON OPEN {} FOR WRITING", output_file_name);
    return 1;
  }

  std::ofstream output_stream {};
  output_stream.open(output_file_name, std::ios::binary);
  if (!output_stream) {
    std::println(stderr, "ERROR: CANNON OPEN {} FOR WRITING", output_file_name);
    return 1;
  }

  output_stream << file_format::wavelet_2d_magic << '\n';

  LFWFEncoder<2> encoder {};
  encoder.create(output_stream, image_size, block_size, color_depth, distortion, predict);
  encoder.encodeStream(puller, output_stream);

  return 0;
}
