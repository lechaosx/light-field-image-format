/******************************************************************************\
* SOUBOR: lfif4d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "decompress.h"
#include "plenoppm.h"
#include "tiler.h"

#include <lfif.h>
#include <lfif_decoder.h>

#include <cmath>

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_stream_name {};
  const char *output_file_mask  {};
  if (!parse_args(argc, argv, input_stream_name, output_file_mask)) {
    return 1;
  }

  ifstream input_stream {};
  input_stream.open(input_stream_name, ios::binary);
  if (!input_stream) {
    cerr << "ERROR: CANNON OPEN " << input_stream_name << " FOR READING\n";
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

  LFIF<4> image {};
  image.open(input_stream);

  vector<PPM> ppm_data(image.size[2] * image.size[3]);
  if (createPPMs(output_file_mask, image.size[0], image.size[1], std::pow<float>(2, image.depth_bits) - 1, ppm_data) < 0) {
    return 3;
  }

  auto puller = [&](const std::array<size_t, 4> &pos) -> std::array<uint16_t, 3> {
    size_t img_index = pos[1] * image.size[0] + pos[0];
    size_t img       = pos[3] * image.size[2] + pos[2];

    return ppm_data[img].get(img_index);
  };

  auto pusher = [&](const std::array<size_t, 4> &pos, const std::array<uint16_t, 3> &RGB) {
    size_t img_index = pos[1] * image.size[0] + pos[0];
    size_t img       = pos[3] * image.size[2] + pos[2];

    ppm_data[img].put(img_index, RGB);
  };

  decodeStreamDCT(image, input_stream, pusher);

  if (std::any_of(std::begin(shift), std::end(shift), [](auto &val) { return val != 0.f; })) {
    for (size_t y {}; y < image.size[3]; y++) {
      for (size_t x {}; x < image.size[2]; x++) {
        auto shiftInputF = [&](const std::array<size_t, 2> &pos) {
          std::array<size_t, 4> whole_image_pos {};

          whole_image_pos[0] = pos[0];
          whole_image_pos[1] = pos[1];
          whole_image_pos[2] = x;
          whole_image_pos[3] = y;

          return puller(whole_image_pos);
        };

        auto shiftOutputF = [&](const std::array<size_t, 2> &pos, const auto &value) {
          std::array<size_t, 4> whole_image_pos {};

          whole_image_pos[0] = pos[0];
          whole_image_pos[1] = pos[1];
          whole_image_pos[2] = x;
          whole_image_pos[3] = y;

          return pusher(whole_image_pos, value);
        };

        shift_image(shiftInputF, shiftOutputF, {image.size[0], image.size[1]}, get_shift_coef({x, y}, {image.size[2], image.size[3]}, shift));
      }
    }
  }

  return 0;
}
