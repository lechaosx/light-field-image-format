/******************************************************************************\
* SOUBOR: xvc_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"
#include "file_mask.h"

#include <ppm.h>

#include <xvcdec.h>

extern "C" {
  #include <libswscale/swscale.h>
}

#include <getopt.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <new>

#include <iostream>

#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <cassert>
#include <bitset>

template <typename T>
inline T endianSwap(T data) {
  uint8_t *ptr = reinterpret_cast<uint8_t *>(&data);
  std::reverse(ptr, ptr + sizeof(T));
  return data;
}

template <typename T>
inline T bigEndianSwap(T data) {
  if (htobe16(1) == 1) {
    return data;
  }
  else {
    return endianSwap(data);
  }
}

template<typename T>
inline T readValueFromStream(std::istream &stream) {
  T dataBE {};
  stream.read(reinterpret_cast<char *>(&dataBE), sizeof(dataBE));
  return bigEndianSwap(dataBE);
}

void print_usage(char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0 << " -i <input-file-name> -o <output-file-mask>" << std::endl;
}

template <typename F>
bool outputPictures(const xvc_decoder_api *xvc_api, xvc_decoder *decoder, F &&callback) {
  xvc_decoded_picture decoded_pic {};

  while (true) {
    const xvc_dec_return_code ret = xvc_api->decoder_get_picture(decoder, &decoded_pic);
    if (ret == XVC_DEC_NO_DECODED_PIC) {
      return true;
    }
    if (ret != XVC_DEC_OK) {
      std::cerr << "xvc output failed: " << xvc_api->xvc_dec_get_error_text(ret) << std::endl;
      return false;
    }
    callback(decoded_pic);
  }
}

template <typename F>
bool decode(const xvc_decoder_api *xvc_api, xvc_decoder *decoder, std::vector<uint8_t> &nal, F &&callback) {
  const xvc_dec_return_code ret = xvc_api->decoder_decode_nal(decoder, nal.data(), nal.size(), 0);
  if (ret != XVC_DEC_OK) {
    std::cerr << "xvc decode failed: " << xvc_api->xvc_dec_get_error_text(ret) << std::endl;
    return false;
  }
  return outputPictures(xvc_api, decoder, callback);
}

template <typename F>
bool flush(const xvc_decoder_api *xvc_api, xvc_decoder *decoder, F &&callback) {
  const xvc_dec_return_code ret = xvc_api->decoder_flush(decoder);
  if (ret != XVC_DEC_OK) {
    std::cerr << "xvc flush failed: " << xvc_api->xvc_dec_get_error_text(ret) << std::endl;
    return false;
  }
  return outputPictures(xvc_api, decoder, callback);
}

int main(int argc, char *argv[]) {
  const char *input_file_name     {};
  const char *output_file_mask    {};

  const xvc_decoder_api *xvc_api  {};
  xvc_decoder_parameters *params  {};
  xvc_decoder *decoder            {};

  std::vector<uint8_t> nal_buffer      {};

  std::ifstream input                   {};

  char opt {};
  while ((opt = getopt(argc, argv, "hi:o:")) >= 0) {
    switch (opt) {
      case 'h':
        print_usage(argv[0]);
        return 0;

      case 'i':
        if (!input_file_name) {
          input_file_name = optarg;
          continue;
        }
      break;

      case 'o':
        if (!output_file_mask) {
          output_file_mask = optarg;
          continue;
        }
      break;

      default:
      break;
    }

    print_usage(argv[0]);
    return 1;
  }

  if (!input_file_name || !output_file_mask) {
    print_usage(argv[0]);
    return 1;
  }

  xvc_api = xvc_decoder_api_get();
  params  = xvc_api->parameters_create();
  xvc_api->parameters_set_default(params);
  if (xvc_api->parameters_check(params) != XVC_DEC_OK) {
    std::exit(1);
  }

  decoder = xvc_api->decoder_create(params);
  if (!decoder) {
    std::cerr << "xvc decoder creation failed" << std::endl;
    xvc_api->parameters_destroy(params);
    return 1;
  }

  input.open(input_file_name, std::ios::binary);
  if (!input) {
    std::cerr << "Could not open " << input_file_name << " for reading\n";
    xvc_api->decoder_destroy(decoder);
    xvc_api->parameters_destroy(params);
    return 1;
  }

  size_t view_counter = 0;
  const size_t output_name_count = get_mask_names_count(output_file_mask, '#');
  if (output_name_count == 0) {
    std::cerr << "output file mask is too large" << std::endl;
    return 1;
  }

  auto saveFrame = [&](xvc_decoded_picture &frame) {
    SwsContext *out_convert_ctx  {};
    std::vector<uint8_t> rgb_frame {};

    rgb_frame.resize(frame.stats.width * frame.stats.height * 3);

    out_convert_ctx = sws_getContext(frame.stats.width, frame.stats.height, AV_PIX_FMT_YUV444P, frame.stats.width, frame.stats.height, AV_PIX_FMT_RGB24, 0, 0, 0, 0);
    if (!out_convert_ctx) {
      std::cerr << "Could not get image conversion context" << std::endl;
      exit(1);
    }

    uint8_t *outData[1]  = { &rgb_frame[0] };
    int outLineSize[1]   = { static_cast<int>(3 * frame.stats.width) };

    sws_scale(out_convert_ctx, reinterpret_cast<const uint8_t *const *>(frame.planes), frame.stride, 0, frame.stats.height, outData, outLineSize);

    if (view_counter >= output_name_count) {
      std::cerr << "file mask cannot represent frame " << view_counter << std::endl;
      exit(1);
    }

    std::string filename = get_name_from_mask(output_file_mask, '#', view_counter);
    view_counter++;

    const std::filesystem::path output_path = filename;
    if (!output_path.parent_path().empty()) {
      std::filesystem::create_directories(output_path.parent_path());
    }

    PPM ppm = PPM::create(output_path, frame.stats.width, frame.stats.height, 255);

    for (size_t pixel = 0; pixel < ppm.width() * ppm.height(); ++pixel) {
      ppm.put(pixel, {
        rgb_frame[pixel * 3 + 0],
        rgb_frame[pixel * 3 + 1],
        rgb_frame[pixel * 3 + 2]
      });
    }
    ppm.flush();

    sws_freeContext(out_convert_ctx);
  };

  bool decoded = true;
  while (decoded) {
    size_t size    {};
    uint8_t nal_size[4] {};

    input.read(reinterpret_cast<char *>(nal_size), 4);
    if (input.gcount() == 0 && input.eof()) {
      break;
    }
    if (input.gcount() != 4) {
      std::cerr << "Unable to read nal size." << std::endl;
      decoded = false;
      break;
    }

    size = static_cast<uint32_t>(nal_size[0]) |
      (static_cast<uint32_t>(nal_size[1]) << 8) |
      (static_cast<uint32_t>(nal_size[2]) << 16) |
      (static_cast<uint32_t>(nal_size[3]) << 24);
    if (size == 0) {
      std::cerr << "Invalid zero-length nal." << std::endl;
      decoded = false;
      break;
    }

    nal_buffer.clear();
    std::array<uint8_t, 64 * 1024> buffer;
    size_t remaining = size;
    try {
      while (remaining != 0) {
        const size_t chunk = std::min(remaining, buffer.size());
        input.read(reinterpret_cast<char *>(buffer.data()), chunk);
        if (static_cast<size_t>(input.gcount()) != chunk) {
          std::cerr << "Unable to read nal." << std::endl;
          decoded = false;
          break;
        }
        nal_buffer.insert(nal_buffer.end(), buffer.begin(), buffer.begin() + chunk);
        remaining -= chunk;
      }
    } catch (const std::bad_alloc &) {
      std::cerr << "nal is too large for memory" << std::endl;
      decoded = false;
    }
    if (!decoded) {
      break;
    }

    decoded = decode(xvc_api, decoder, nal_buffer, saveFrame);
  }

  if (decoded) {
    decoded = flush(xvc_api, decoder, saveFrame);
  }

  xvc_api->decoder_destroy(decoder);
  xvc_api->parameters_destroy(params);

  if (decoded && view_counter == 0) {
    std::cerr << "xvc stream contained no pictures" << std::endl;
    decoded = false;
  }

  return decoded ? 0 : 1;
}
