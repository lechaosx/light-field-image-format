/******************************************************************************\
* SOUBOR: lfif4d_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>

#include <ppm.h>
#include <lfif_decoder.h>
#include <lfwf_decoder.h>

#include <decompress.h>
#include <plenoppm.h>

using namespace std;

int main(int argc, char *argv[]) {
  const char *input_stream_name {};
  const char *output_file_name  {};
  if (!parse_args(argc, argv, input_stream_name, output_file_name)) {
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

  if (magic_number != std::string("LFIF-2D")) {
    throw std::runtime_error("Magic number does not match!");
  }

  LFWFDecoder<2> decoder {};
  decoder.open(input_stream);

  PPM ppm_image {};
  if (ppm_image.createPPM(output_file_name, decoder.size[0], decoder.size[1], std::pow<float>(2, decoder.depth_bits) - 1) < 0) {
    return 3;
  }

  auto pusher = [&](const std::array<size_t, 2> &pos, const std::array<uint16_t, 3> &RGB) {
    ppm_image.put(pos[1] * decoder.size[0] + pos[0], RGB);
  };

  decoder.decodeStream(input_stream, pusher);

  return 0;
}
