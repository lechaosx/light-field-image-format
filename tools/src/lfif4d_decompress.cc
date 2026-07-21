#include <fstream>
#include <print>
#include <vector>

#include <lfif_decoder.h>

#include <decompress.h>
#include <plenoppm.h>
#include <tiler.h>

int main(int argc, char *argv[]) {
  const char *input_stream_name {};
  const char *output_file_mask  {};
  if (!parse_args(argc, argv, input_stream_name, output_file_mask)) {
    return 1;
  }

  std::ifstream input_stream {};
  input_stream.open(input_stream_name, std::ios::binary);
  if (!input_stream) {
    std::println(stderr, "ERROR: CANNON OPEN {} FOR READING", input_stream_name);
    return 1;
  }

  std::string magic_number {};
  input_stream >> magic_number;
  input_stream.ignore();

  if (magic_number != std::string("LFIF-4D")) {
    throw std::runtime_error("Magic number does not match!");
  }

  std::array<int64_t, 2> shift {};
  input_stream >> shift[0];
  input_stream >> shift[1];
  input_stream.ignore();

  LFIFDecoder<4> decoder {};
  decoder.open(input_stream);

  std::vector<PPM> ppm_data(decoder.header.size[2] * decoder.header.size[3]);
  if (createPPMs(output_file_mask, decoder.header.size[0], decoder.header.size[1], (1u << decoder.header.depth_bits) - 1, ppm_data) < 0) {
    return 3;
  }

  auto puller = [&](const std::array<size_t, 4> &pos) -> std::array<uint16_t, 3> {
    size_t img_index = pos[1] * decoder.header.size[0] + pos[0];
    size_t img       = pos[3] * decoder.header.size[2] + pos[2];

    return ppm_data[img].get(img_index);
  };

  auto pusher = [&](const std::array<size_t, 4> &pos, const std::array<uint16_t, 3> &RGB) {
    size_t img_index = pos[1] * decoder.header.size[0] + pos[0];
    size_t img       = pos[3] * decoder.header.size[2] + pos[2];

    ppm_data[img].put(img_index, RGB);
  };

  decoder.decodeStream(input_stream, pusher);

  if (std::ranges::any_of(shift, [](auto val) { return val != 0; })) {
    for (size_t y {}; y < decoder.header.size[3]; y++) {
      for (size_t x {}; x < decoder.header.size[2]; x++) {
        auto shiftInputF = [&](const std::array<size_t, 2> &pos) {
          std::array<size_t, 4> whole_image_pos { pos[0], pos[1], x, y };
          return puller(whole_image_pos);
        };

        auto shiftOutputF = [&](const std::array<size_t, 2> &pos, const auto &value) {
          std::array<size_t, 4> whole_image_pos { pos[0], pos[1], x, y };
          return pusher(whole_image_pos, value);
        };

        shift_image(shiftInputF, shiftOutputF, {decoder.header.size[0], decoder.header.size[1]}, get_shift_coef({x, y}, {decoder.header.size[2], decoder.header.size[3]}, shift));
      }
    }
  }

  return 0;
}
