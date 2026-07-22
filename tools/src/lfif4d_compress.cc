#include <bit>
#include <charconv>
#include <cmath>
#include <fstream>
#include <print>
#include <vector>

#include <lfwf_encoder.h>

#include <compress.h>
#include <file_format.h>
#include <plenoppm.h>
#include <tiler.h>

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_name {};

  uint8_t distortion {};
  bool    predict    {};
  bool    do_shift   {};

  if (!parse_args(argc, argv, input_file_mask, output_file_name, distortion, predict, do_shift)) {
    return 1;
  }

  std::vector<PPM> ppm_data {};

  uint64_t width         {};
  uint64_t height        {};
  uint64_t image_count   {};
  uint32_t max_rgb_value {};

  if (mapPPMs(input_file_mask, width, height, max_rgb_value, ppm_data) < 0) {
    std::println(stderr, "ERROR: IMAGES PROPERTIES MISMATCH");
    return 2;
  }

  image_count = ppm_data.size();
  if (!image_count) {
    std::println(stderr, "ERROR: NO IMAGES LOADED");
    return 2;
  }

  size_t  side        = std::sqrt(image_count);
  uint8_t color_depth = std::bit_width(max_rgb_value);

  std::array<uint64_t, 4> image_size { width, height, side, side };
  std::array<uint64_t, 4> block_size {};
  if (optind + 4 == argc) {
    for (size_t i { 0 }; i < 4; i++) {
      std::string_view arg { argv[optind] };
      size_t tmp {};
      auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), tmp);
      if (ec != std::errc{}) {
        block_size = {side, side, side, side};
        break;
      }
      block_size[i] = tmp;
      optind++;
    }
  }
  else {
    block_size = {side, side, side, side};
  }

  auto puller = [&](const std::array<size_t, 4> &pos) -> std::array<uint16_t, 3> {
    size_t img_index = pos[1] * image_size[0] + pos[0];
    size_t img       = pos[3] * image_size[2] + pos[2];

    return ppm_data[img].get(img_index);
  };

  auto pusher = [&](const std::array<size_t, 4> &pos, const std::array<uint16_t, 3> &RGB) {
    size_t img_index = pos[1] * image_size[0] + pos[0];
    size_t img       = pos[3] * image_size[2] + pos[2];

    ppm_data[img].put(img_index, RGB);
  };

  std::array<int64_t, 2> shift {};

  if (do_shift) {
    int64_t width_shift  {static_cast<int64_t>((width  * 2) / side)};
    int64_t height_shift {static_cast<int64_t>((height * 2) / side)};

    shift = find_best_shift_params(puller, image_size, {-width_shift, -height_shift}, {width_shift, height_shift}, 10, 8);

    for (size_t y = 0; y < side; y++) {
      for (size_t x = 0; x < side; x++) {
        std::array<size_t, 4> whole_image_pos {0, 0, x, y};

        auto shiftInputF = [&](const std::array<size_t, 2> &pos) {
          whole_image_pos[0] = pos[0];
          whole_image_pos[1] = pos[1];
          return puller(whole_image_pos);
        };

        auto shiftOutputF = [&](const std::array<size_t, 2> &pos, const auto &value) {
          whole_image_pos[0] = pos[0];
          whole_image_pos[1] = pos[1];
          return pusher(whole_image_pos, value);
        };

        shift_image(shiftInputF, shiftOutputF, {width, height}, get_shift_coef({x, y}, {side, side}, shift));
      }
    }
  }

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

  output_stream << file_format::wavelet_4d_magic << '\n';
  output_stream << shift[0] << " " << shift[1] << "\n";

  LFWFEncoder<4> encoder {};
  encoder.create(output_stream, image_size, block_size, color_depth, distortion, predict);

  encoder.encodeStream(puller, output_stream);

  return 0;
}
