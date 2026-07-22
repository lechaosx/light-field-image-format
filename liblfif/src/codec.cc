#include <lfif/codec.h>

#include <lfif_decoder.h>
#include <lfif_encoder.h>
#include <lfwf_decoder.h>
#include <lfwf_encoder.h>

#include <array>
#include <cstddef>
#include <ios>
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace lfif {
namespace {

size_t pixelCount(const Header &header) {
  size_t count = 1;
  for (const uint64_t extent : header.extents) {
    count *= static_cast<size_t>(extent);
  }
  return count;
}

template <size_t D>
std::array<size_t, D> extents(const std::vector<uint64_t> &values) {
  std::array<size_t, D> result;
  for (size_t i = 0; i < D; ++i) {
    result[i] = static_cast<size_t>(values[i]);
  }
  return result;
}

template <size_t D>
size_t pixelIndex(const std::array<size_t, D> &position, const std::array<size_t, D> &size) {
  size_t index = 0;
  for (size_t i = D; i > 0; --i) {
    index *= size[i - 1];
    index += position[i - 1];
  }
  return index;
}

template <size_t D, typename Encoder>
std::string encodePayload(const Header &header, std::span<const Pixel> pixels) {
  const auto image_size = extents<D>(header.extents);
  Encoder encoder;
  encoder.size = image_size;
  encoder.block_size = extents<D>(header.block_extents);
  encoder.depth_bits = header.sample_depth;
  encoder.discarded_bits = header.discarded_bits;
  encoder.predicted = header.prediction;

  std::ostringstream payload(std::ios::binary);
  encoder.encodeStream(
      [&](const std::array<size_t, D> &position) { return pixels[pixelIndex(position, image_size)]; },
      payload);
  return payload.str();
}

template <size_t D>
std::string encodePayload(const Header &header, std::span<const Pixel> pixels) {
  if (header.transform == Transform::wavelet) {
    return encodePayload<D, LFWFEncoder<D>>(header, pixels);
  }
  return encodePayload<D, LFIFEncoder<D>>(header, pixels);
}

template <size_t D, typename Decoder>
std::vector<Pixel> decodePayload(const Header &header, std::istream &payload) {
  const auto image_size = extents<D>(header.extents);
  Decoder decoder;
  decoder.size = image_size;
  decoder.block_size = extents<D>(header.block_extents);
  decoder.depth_bits = header.sample_depth;
  decoder.discarded_bits = header.discarded_bits;
  decoder.predicted = header.prediction;

  std::vector<Pixel> pixels(pixelCount(header));
  decoder.decodeStream(payload, [&](const std::array<size_t, D> &position, const Pixel &pixel) {
    pixels[pixelIndex(position, image_size)] = pixel;
  });
  return pixels;
}

template <size_t D>
std::vector<Pixel> decodePayload(const Header &header, std::istream &payload) {
  if (header.transform == Transform::wavelet) {
    return decodePayload<D, LFWFDecoder<D>>(header, payload);
  }
  return decodePayload<D, LFIFDecoder<D>>(header, payload);
}

std::string encodePayload(const Header &header, std::span<const Pixel> pixels) {
  switch (header.extents.size()) {
    case 2: return encodePayload<2>(header, pixels);
    case 3: return encodePayload<3>(header, pixels);
    case 4: return encodePayload<4>(header, pixels);
    default: throw std::invalid_argument("codec supports two to four dimensions");
  }
}

std::vector<Pixel> decodePayload(const Header &header, std::istream &payload) {
  switch (header.extents.size()) {
    case 2: return decodePayload<2>(header, payload);
    case 3: return decodePayload<3>(header, payload);
    case 4: return decodePayload<4>(header, payload);
    default: throw std::runtime_error("codec supports two to four dimensions");
  }
}

} // namespace

Header writeImage(std::ostream &output, Header header, std::span<const Pixel> pixels) {
  header.payload_size = 0;
  serializeHeader(header);
  if (header.extents.size() < 2 || header.extents.size() > 4) {
    throw std::invalid_argument("codec supports two to four dimensions");
  }
  if (pixels.size() != pixelCount(header)) {
    throw std::invalid_argument("pixel count does not match image extents");
  }

  const std::string payload = encodePayload(header, pixels);
  header.payload_size = payload.size();
  const std::vector<uint8_t> encoded_header = serializeHeader(header);
  output.write(reinterpret_cast<const char *>(encoded_header.data()), encoded_header.size());
  output.write(payload.data(), payload.size());
  if (!output) {
    throw std::ios_base::failure("failed to write LFIF image");
  }
  return header;
}

DecodedImage readImage(std::istream &input) {
  Header header = parseHeader(input);
  if (header.extents.size() < 2 || header.extents.size() > 4) {
    throw std::runtime_error("codec supports two to four dimensions");
  }
  if (header.payload_size > std::numeric_limits<size_t>::max()
      || header.payload_size > static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max())) {
    throw std::runtime_error("LFIF payload is too large");
  }

  std::string encoded_payload(static_cast<size_t>(header.payload_size), '\0');
  input.read(encoded_payload.data(), static_cast<std::streamsize>(encoded_payload.size()));
  if (input.gcount() != static_cast<std::streamsize>(encoded_payload.size())) {
    throw std::runtime_error("truncated LFIF payload");
  }

  std::istringstream payload(encoded_payload, std::ios::binary);
  return {header, decodePayload(header, payload)};
}

} // namespace lfif
