#include <lfif/format.h>

#include <fstream>
#include <stdexcept>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    throw std::invalid_argument("invalid arguments");
  }

  lfif::Header header {
      .extents = {1, 1},
      .block_extents = {1, 1},
      .sample_depth = 8,
      .channels = 3,
      .sample_format = lfif::SampleFormat::unsigned_integer,
      .transform = lfif::Transform::wavelet,
      .entropy_codec = lfif::EntropyCodec::cabac,
      .color_space = lfif::ColorSpace::rgb,
      .payload_size = 8,
  };
  const std::vector<uint8_t> bytes = lfif::serializeHeader(header);
  std::ofstream output(argv[1], std::ios::binary);
  output.exceptions(std::ios::badbit | std::ios::failbit);
  output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
  output.close();
}
