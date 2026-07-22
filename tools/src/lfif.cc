#include <lfif/codec.h>
#include <lfif/format.h>
#include <ppm.h>

#include <algorithm>
#include <bit>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct CompressOptions {
  std::string input;
  std::string output;
  std::vector<uint64_t> view_shape;
  std::vector<uint64_t> block_extents;
  lfif::Transform transform {lfif::Transform::wavelet};
  uint8_t discarded_bits {};
  bool prediction {};
};

void printUsage(std::ostream &output) {
  output
      << "Usage:\n"
      << "  lfif compress INPUT OUTPUT [--shape EXTENTS] [--block EXTENTS]\n"
      << "      [--transform wavelet|dct] [--discarded-bits N] [--predict]\n"
      << "  lfif decompress INPUT OUTPUT_MASK\n"
      << "  lfif inspect INPUT\n";
}

uint64_t parseNumber(std::string_view text, std::string_view name) {
  uint64_t value = 0;
  const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  if (text.empty() || result.ec != std::errc {} || result.ptr != text.data() + text.size()) {
    throw std::invalid_argument("invalid " + std::string(name) + ": " + std::string(text));
  }
  return value;
}

std::vector<uint64_t> parseExtents(std::string_view text, std::string_view name) {
  std::vector<uint64_t> result;
  while (!text.empty()) {
    const size_t separator = text.find('x');
    const std::string_view part = text.substr(0, separator);
    const uint64_t extent = parseNumber(part, name);
    if (extent == 0) {
      throw std::invalid_argument(std::string(name) + " must be nonzero");
    }
    result.push_back(extent);
    if (separator == std::string_view::npos) {
      text = {};
    } else {
      text.remove_prefix(separator + 1);
      if (text.empty()) {
        throw std::invalid_argument("invalid " + std::string(name));
      }
    }
  }
  if (result.empty()) {
    throw std::invalid_argument("missing " + std::string(name));
  }
  return result;
}

size_t checkedProduct(const std::vector<uint64_t> &values) {
  size_t product = 1;
  for (const uint64_t value : values) {
    if (value > std::numeric_limits<size_t>::max()
        || product > std::numeric_limits<size_t>::max() / static_cast<size_t>(value)) {
      throw std::length_error("dimension product overflow");
    }
    product *= static_cast<size_t>(value);
  }
  return product;
}

std::string expandMask(const std::string &mask, size_t index, size_t count) {
  std::vector<size_t> positions;
  for (size_t i = 0; i < mask.size(); ++i) {
    if (mask[i] == '#') {
      positions.push_back(i);
    }
  }
  if (count == 1 && positions.empty()) {
    return mask;
  }
  if (positions.empty()) {
    throw std::invalid_argument("multiple images require a # output or input mask");
  }

  std::string digits = std::to_string(index);
  if (digits.size() > positions.size()) {
    throw std::invalid_argument("file mask has too few # characters");
  }
  digits.insert(digits.begin(), positions.size() - digits.size(), '0');
  std::string result = mask;
  for (size_t i = 0; i < positions.size(); ++i) {
    result[positions[i]] = digits[i];
  }
  return result;
}

void createParentDirectory(const std::string &file_name) {
  const std::filesystem::path parent = std::filesystem::path(file_name).parent_path();
  if (parent.empty()) {
    return;
  }
  std::error_code error;
  std::filesystem::create_directories(parent, error);
  if (error) {
    throw std::runtime_error("cannot create output directory: " + error.message());
  }
}

CompressOptions parseCompressOptions(int argc, char *argv[]) {
  if (argc < 4) {
    throw std::invalid_argument("compress requires INPUT and OUTPUT");
  }
  CompressOptions options;
  options.input = argv[2];
  options.output = argv[3];
  std::string block;
  for (int i = 4; i < argc; ++i) {
    const std::string_view option = argv[i];
    if (option == "--predict") {
      options.prediction = true;
    } else if (option == "--shape" && i + 1 < argc) {
      options.view_shape = parseExtents(argv[++i], "shape");
    } else if (option == "--block" && i + 1 < argc) {
      block = argv[++i];
    } else if (option == "--discarded-bits" && i + 1 < argc) {
      const uint64_t value = parseNumber(argv[++i], "discarded bits");
      if (value > std::numeric_limits<uint8_t>::max()) {
        throw std::invalid_argument("discarded bits are too large");
      }
      options.discarded_bits = static_cast<uint8_t>(value);
    } else if (option == "--transform" && i + 1 < argc) {
      const std::string_view transform = argv[++i];
      if (transform == "wavelet") {
        options.transform = lfif::Transform::wavelet;
      } else if (transform == "dct") {
        options.transform = lfif::Transform::dct;
      } else {
        throw std::invalid_argument("transform must be wavelet or dct");
      }
    } else {
      throw std::invalid_argument("unknown or incomplete option: " + std::string(option));
    }
  }
  if (options.view_shape.size() > 2) {
    throw std::invalid_argument("shape supports at most two view dimensions");
  }
  if (!block.empty()) {
    options.block_extents = parseExtents(block, "block extents");
  }
  return options;
}

int compress(int argc, char *argv[]) {
  CompressOptions options = parseCompressOptions(argc, argv);
  const size_t image_count = checkedProduct(options.view_shape);
  expandMask(options.input, image_count - 1, image_count);

  uint64_t width = 0;
  uint64_t height = 0;
  uint32_t color_depth = 0;
  size_t pixels_per_image = 0;
  std::vector<lfif::Pixel> pixels;
  for (size_t image = 0; image < image_count; ++image) {
    const std::string file_name = expandMask(options.input, image, image_count);
    PPM ppm;
    if (ppm.mmapPPM(file_name.c_str()) < 0) {
      throw std::runtime_error("cannot read PPM: " + file_name);
    }
    if (!std::has_single_bit(ppm.color_depth() + 1)) {
      throw std::runtime_error("PPM maxval must be one less than a power of two");
    }
    if (image == 0) {
      width = ppm.width();
      height = ppm.height();
      color_depth = ppm.color_depth();
      std::vector<uint64_t> image_extents {width, height};
      image_extents.insert(image_extents.end(), options.view_shape.begin(), options.view_shape.end());
      pixels.reserve(checkedProduct(image_extents));
      pixels_per_image = checkedProduct({width, height});
    } else if (ppm.width() != width || ppm.height() != height || ppm.color_depth() != color_depth) {
      throw std::runtime_error("PPM dimensions or sample depths do not match");
    }
    for (size_t pixel = 0; pixel < pixels_per_image; ++pixel) {
      pixels.push_back(ppm.get(pixel));
    }
  }

  std::vector<uint64_t> image_extents {width, height};
  image_extents.insert(image_extents.end(), options.view_shape.begin(), options.view_shape.end());
  if (options.block_extents.empty()) {
    for (const uint64_t extent : image_extents) {
      options.block_extents.push_back(std::min<uint64_t>(8, extent));
    }
  }
  if (options.block_extents.size() != image_extents.size()) {
    throw std::invalid_argument("block extent count must match image dimensions");
  }

  lfif::Header header {
      .extents = image_extents,
      .block_extents = options.block_extents,
      .sample_depth = static_cast<uint8_t>(std::bit_width(color_depth)),
      .channels = 3,
      .sample_format = lfif::SampleFormat::unsigned_integer,
      .transform = options.transform,
      .entropy_codec = lfif::EntropyCodec::cabac,
      .color_space = lfif::ColorSpace::rgb,
      .discarded_bits = options.discarded_bits,
      .prediction = options.prediction,
  };

  std::ostringstream encoded(std::ios::binary);
  lfif::writeImage(encoded, header, pixels);
  createParentDirectory(options.output);
  std::ofstream output(options.output, std::ios::binary);
  const std::string bytes = encoded.str();
  output.write(bytes.data(), bytes.size());
  output.flush();
  output.close();
  if (!output) {
    throw std::runtime_error("cannot write output: " + options.output);
  }
  return 0;
}

int decompress(const std::string &input_name, const std::string &output_mask) {
  std::ifstream input(input_name, std::ios::binary);
  if (!input) {
    throw std::runtime_error("cannot read input: " + input_name);
  }
  const lfif::DecodedImage image = lfif::readImage(input);
  const size_t image_count = checkedProduct(
      std::vector<uint64_t>(image.header.extents.begin() + 2, image.header.extents.end()));
  expandMask(output_mask, image_count - 1, image_count);
  const size_t pixels_per_image = static_cast<size_t>(image.header.extents[0] * image.header.extents[1]);
  const uint32_t color_depth = image.header.sample_depth == 16
      ? 65535
      : (uint32_t {1} << image.header.sample_depth) - 1;

  for (size_t view = 0; view < image_count; ++view) {
    const std::string output_name = expandMask(output_mask, view, image_count);
    createParentDirectory(output_name);
    PPM ppm;
    if (ppm.createPPM(
            output_name.c_str(), image.header.extents[0], image.header.extents[1], color_depth) < 0) {
      throw std::runtime_error("cannot create PPM: " + output_name);
    }
    for (size_t pixel = 0; pixel < pixels_per_image; ++pixel) {
      ppm.put(pixel, image.pixels[view * pixels_per_image + pixel]);
    }
  }
  return 0;
}

const char *transformName(lfif::Transform transform) {
  return transform == lfif::Transform::wavelet ? "wavelet" : "dct";
}

void printExtents(const std::vector<uint64_t> &extents) {
  for (size_t i = 0; i < extents.size(); ++i) {
    if (i != 0) {
      std::cout << 'x';
    }
    std::cout << extents[i];
  }
  std::cout << '\n';
}

int inspect(const std::string &input_name) {
  std::ifstream input(input_name, std::ios::binary);
  if (!input) {
    throw std::runtime_error("cannot read input: " + input_name);
  }
  const lfif::Header header = lfif::parseHeader(input);
  const std::streampos payload_position = input.tellg();
  input.seekg(0, std::ios::end);
  const std::streampos end_position = input.tellg();
  if (payload_position < 0 || end_position < payload_position
      || static_cast<uint64_t>(end_position - payload_position) < header.payload_size) {
    throw std::runtime_error("truncated LFIF payload");
  }
  std::cout << "dimensions: " << header.extents.size() << '\n';
  std::cout << "extents: ";
  printExtents(header.extents);
  std::cout << "block extents: ";
  printExtents(header.block_extents);
  std::cout << "sample depth: " << static_cast<unsigned>(header.sample_depth) << '\n';
  std::cout << "channels: " << static_cast<unsigned>(header.channels) << '\n';
  std::cout << "sample format: unsigned integer\n";
  std::cout << "transform: " << transformName(header.transform) << '\n';
  std::cout << "entropy codec: CABAC\n";
  std::cout << "color space: RGB\n";
  std::cout << "prediction: " << (header.prediction ? "yes" : "no") << '\n';
  std::cout << "discarded bits: " << static_cast<unsigned>(header.discarded_bits) << '\n';
  std::cout << "disparity compensation: "
            << (header.disparity_compensated ? "yes" : "no") << '\n';
  std::cout << "disparity shifts: " << header.disparity_shift[0]
            << ", " << header.disparity_shift[1] << '\n';
  std::cout << "payload bytes: " << header.payload_size << '\n';
  return 0;
}

}

int main(int argc, char *argv[]) {
  try {
    if (argc < 2 || std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h") {
      printUsage(argc < 2 ? std::cerr : std::cout);
      return argc < 2 ? 1 : 0;
    }

    const std::string_view command = argv[1];
    if (command == "compress") {
      return compress(argc, argv);
    }
    if (command == "decompress" && argc == 4) {
      return decompress(argv[2], argv[3]);
    }
    if (command == "inspect" && argc == 3) {
      return inspect(argv[2]);
    }
    throw std::invalid_argument("invalid command or argument count");
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}
