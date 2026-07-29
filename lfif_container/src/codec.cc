#include <lfif/codec.h>

#include <lfif_decoder.h>
#include <lfif_encoder.h>
#include <lfwf_decoder.h>
#include <lfwf_encoder.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <ios>
#include <istream>
#include <limits>
#include <new>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

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

template <size_t D, typename Encoder, typename PixelReader>
std::string encodePayload(const Header &header, PixelReader &pixels) {
  const auto image_size = extents<D>(header.extents);
  Encoder encoder;
  encoder.size = image_size;
  encoder.block_size = extents<D>(header.block_extents);
  encoder.depth_bits = header.sample_depth;
  encoder.discarded_bits = header.discarded_bits;
  encoder.predicted = header.prediction;

  std::ostringstream payload(std::ios::binary);
  encoder.encodeStream(
      [&](const std::array<size_t, D> &position) {
        return pixels(pixelIndex(position, image_size));
      },
      payload);
  return payload.str();
}

template <size_t D>
std::string encodePayload(
    const Header &header, const std::function<Pixel(size_t)> &pixels) {
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

std::string encodePayload(
    const Header &header, const std::function<Pixel(size_t)> &pixels) {
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

template <typename PixelReader>
std::vector<Pixel> applyDisparity(
    const Header &header, PixelReader &pixels, bool inverse) {
  const std::array<size_t, 4> size = extents<4>(header.extents);
  std::vector<Pixel> result(pixelCount(header));
  const auto multiply = [](int64_t left, int64_t right) {
    constexpr int64_t minimum = std::numeric_limits<int64_t>::min();
    constexpr int64_t maximum = std::numeric_limits<int64_t>::max();
    if (left == 0 || right == 0) {
      return int64_t {0};
    }
    if ((left == -1 && right == minimum) || (right == -1 && left == minimum)
        || (left > 0 && right > 0 && left > maximum / right)
        || (left > 0 && right < 0 && right < minimum / left)
        || (left < 0 && right > 0 && left < minimum / right)
        || (left < 0 && right < 0 && left < maximum / right)) {
      throw std::overflow_error("disparity shift overflow");
    }
    return left * right;
  };
  for (size_t view_y = 0; view_y < size[3]; ++view_y) {
    for (size_t view_x = 0; view_x < size[2]; ++view_x) {
      const int64_t horizontal_shift = multiply(
          static_cast<int64_t>(view_x) - static_cast<int64_t>(size[2] / 2),
          header.disparity_shift[0]);
      const int64_t vertical_shift = multiply(
          static_cast<int64_t>(view_y) - static_cast<int64_t>(size[3] / 2),
          header.disparity_shift[1]);

      const auto wrap = [](size_t coordinate, int64_t shift, size_t extent) {
        const uint64_t magnitude = shift < 0
            ? static_cast<uint64_t>(-(shift + 1)) + 1
            : static_cast<uint64_t>(shift);
        const size_t offset = magnitude % extent;
        if (shift < 0) {
          return coordinate >= offset
              ? coordinate - offset
              : extent - (offset - coordinate);
        }
        return coordinate >= extent - offset
            ? coordinate - (extent - offset)
            : coordinate + offset;
      };

      for (size_t y = 0; y < size[1]; ++y) {
        for (size_t x = 0; x < size[0]; ++x) {
          const std::array<size_t, 4> destination {x, y, view_x, view_y};
          const std::array<size_t, 4> source {
              wrap(x, horizontal_shift, size[0]),
              wrap(y, vertical_shift, size[1]),
              view_x,
              view_y,
          };
          if (inverse) {
            result[pixelIndex(source, size)] = pixels(pixelIndex(destination, size));
          } else {
            result[pixelIndex(destination, size)] = pixels(pixelIndex(source, size));
          }
        }
      }
    }
  }
  return result;
}

}

Header writeImage(std::ostream &output, Header header, std::span<const Pixel> pixels) {
  const std::function<Pixel(size_t)> source =
      [pixels](size_t index) { return pixels[index]; };
  return writeImage(output, std::move(header), pixels.size(), source);
}

Header writeImage(
    std::ostream &output,
    Header header,
    size_t input_pixel_count,
    const std::function<Pixel(size_t)> &pixels) {
  header.payload_size = 0;
  serializeHeader(header);
  if (header.extents.size() < 2 || header.extents.size() > 4) {
    throw std::invalid_argument("codec supports two to four dimensions");
  }
  if (input_pixel_count != pixelCount(header)) {
    throw std::invalid_argument("pixel count does not match image extents");
  }
  const uint16_t maximum_sample = header.sample_depth == 16
      ? std::numeric_limits<uint16_t>::max()
      : static_cast<uint16_t>((uint32_t {1} << header.sample_depth) - 1);
  const auto checked_pixels = [&](size_t index) {
    const Pixel pixel = pixels(index);
    if (std::any_of(pixel.begin(), pixel.end(), [maximum_sample](uint16_t sample) {
          return sample > maximum_sample;
        })) {
      throw std::invalid_argument("pixel sample exceeds declared depth");
    }
    return pixel;
  };

  std::string payload;
  if (header.disparity_compensated) {
    const std::vector<Pixel> compensated =
        applyDisparity(header, checked_pixels, false);
    const std::function<Pixel(size_t)> compensated_pixels =
        [&compensated](size_t index) { return compensated[index]; };
    payload = encodePayload(header, compensated_pixels);
  } else {
    const std::function<Pixel(size_t)> source = checked_pixels;
    payload = encodePayload(header, source);
  }
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

  std::string encoded_payload;
  std::array<char, 64 * 1024> buffer;
  uint64_t remaining = header.payload_size;
  try {
    while (remaining != 0) {
      const size_t chunk = static_cast<size_t>(std::min<uint64_t>(remaining, buffer.size()));
      input.read(buffer.data(), static_cast<std::streamsize>(chunk));
      if (input.gcount() != static_cast<std::streamsize>(chunk)) {
        throw std::runtime_error("truncated LFIF payload");
      }
      encoded_payload.append(buffer.data(), chunk);
      remaining -= chunk;
    }
  } catch (const std::bad_alloc &) {
    throw std::runtime_error("LFIF payload is too large for memory");
  }

  std::istringstream payload(encoded_payload, std::ios::binary);
  try {
    std::vector<Pixel> pixels = decodePayload(header, payload);
    if (header.disparity_compensated) {
      const auto source = [&pixels](size_t index) { return pixels[index]; };
      pixels = applyDisparity(header, source, true);
    }
    return {header, std::move(pixels)};
  } catch (const std::bad_alloc &) {
    throw std::runtime_error("LFIF image is too large for memory");
  }
}

}
