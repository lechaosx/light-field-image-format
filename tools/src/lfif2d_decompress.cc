#include <fstream>
#include <print>

#include <ppm.h>
#include <lfwf_decoder.h>

#include <decompress.h>
#include <file_format.h>
#include <plenoppm.h>

int main(int argc, char *argv[]) {
  const char *input_stream_name {};
  const char *output_file_name  {};
  if (!parse_args(argc, argv, input_stream_name, output_file_name)) {
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

  if (!file_format::is_wavelet_2d(magic_number)) {
    std::println(stderr, "ERROR: UNSUPPORTED 2D FORMAT {}", magic_number);
    return 2;
  }

  LFWFDecoder<2> decoder {};
  decoder.open(input_stream);

  PPM ppm_image {};
  if (ppm_image.createPPM(output_file_name, decoder.header.size[0], decoder.header.size[1], (1u << decoder.header.depth_bits) - 1) < 0) {
    return 3;
  }

  auto pusher = [&](const std::array<size_t, 2> &pos, const std::array<uint16_t, 3> &RGB) {
    ppm_image.put(pos[1] * decoder.header.size[0] + pos[0], RGB);
  };

  decoder.decodeStream(input_stream, pusher);

  return 0;
}
