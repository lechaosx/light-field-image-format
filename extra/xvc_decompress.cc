/******************************************************************************\
* SOUBOR: xvc_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "ffmpeg_raii.h"
#include "file_mask.h"
#include "plenoppm.h"

#include <ppm.h>

#include <xvcdec.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>

void print_usage(char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0 << " -i <input-file-name> -o <output-file-mask>"
            << std::endl;
}

template <typename F>
void outputPictures(const xvc_decoder_api *xvc_api, xvc_decoder *decoder,
                    F &&callback) {
  xvc_decoded_picture decoded_pic{};

  while (true) {
    const xvc_dec_return_code ret =
        xvc_api->decoder_get_picture(decoder, &decoded_pic);
    if (ret == XVC_DEC_NO_DECODED_PIC) {
      return;
    }
    if (ret != XVC_DEC_OK) {
      throw std::runtime_error(std::string("xvc output failed: ") +
                               xvc_api->xvc_dec_get_error_text(ret));
    }
    callback(decoded_pic);
  }
}

template <typename F>
void decode(const xvc_decoder_api *xvc_api, xvc_decoder *decoder,
            std::vector<uint8_t> &nal, F &&callback) {
  const xvc_dec_return_code ret =
      xvc_api->decoder_decode_nal(decoder, nal.data(), nal.size(), 0);
  if (ret != XVC_DEC_OK) {
    throw std::runtime_error(std::string("xvc decode failed: ") +
                             xvc_api->xvc_dec_get_error_text(ret));
  }
  outputPictures(xvc_api, decoder, callback);
}

template <typename F>
void flush(const xvc_decoder_api *xvc_api, xvc_decoder *decoder, F &&callback) {
  const xvc_dec_return_code ret = xvc_api->decoder_flush(decoder);
  if (ret != XVC_DEC_OK) {
    throw std::runtime_error(std::string("xvc flush failed: ") +
                             xvc_api->xvc_dec_get_error_text(ret));
  }
  outputPictures(xvc_api, decoder, callback);
}

int main(int argc, char *argv[]) {
  const char *input_file_name{};
  const char *output_file_mask{};

  std::vector<uint8_t> nal_buffer{};

  std::ifstream input{};

  char opt{};
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
    throw std::invalid_argument("invalid arguments");
  }

  if (!input_file_name || !output_file_mask) {
    print_usage(argv[0]);
    throw std::invalid_argument("input and output are required");
  }

  const xvc_decoder_api *xvc_api = xvc_decoder_api_get();
  std::unique_ptr<xvc_decoder_parameters,
                  decltype(xvc_api->parameters_destroy)> params {
      xvc_api->parameters_create(), xvc_api->parameters_destroy};
  if (!params) {
    throw std::runtime_error("xvc parameter creation failed");
  }

  xvc_api->parameters_set_default(params.get());
  const xvc_dec_return_code parameter_result =
      xvc_api->parameters_check(params.get());
  if (parameter_result != XVC_DEC_OK) {
    throw std::runtime_error(std::string("xvc parameter error: ") +
                             xvc_api->xvc_dec_get_error_text(parameter_result));
  }

  std::unique_ptr<xvc_decoder, decltype(xvc_api->decoder_destroy)> decoder {
      xvc_api->decoder_create(params.get()), xvc_api->decoder_destroy};
  if (!decoder) {
    throw std::runtime_error("xvc decoder creation failed");
  }

  input.open(input_file_name, std::ios::binary);
  if (!input) {
    throw std::ios_base::failure("could not open " +
                                 std::string(input_file_name) + " for reading");
  }

  size_t view_counter = 0;
  const FileMask output_names{output_file_mask};
  const size_t output_name_count = output_names.count();
  if (output_name_count == 0) {
    throw std::invalid_argument("output file mask is too large");
  }

  auto saveFrame = [&](xvc_decoded_picture &frame) {
    if (frame.stats.width <= 0 || frame.stats.height <= 0
        || frame.stats.width > std::numeric_limits<int>::max() / 3) {
      throw std::runtime_error("decoded frame dimensions exceed codec limits");
    }
    const int width = static_cast<int>(frame.stats.width);
    const int height = static_cast<int>(frame.stats.height);
    const size_t frame_pixels = static_cast<size_t>(width) * height;
    if (frame_pixels > std::numeric_limits<size_t>::max() / 3) {
      throw std::length_error("decoded frame is too large");
    }
    std::vector<uint8_t> rgb_frame(frame_pixels * 3);

    ffmpeg::ScaleContext out_convert_ctx{
        sws_getContext(width, height, AV_PIX_FMT_YUV444P, width,
                       height, AV_PIX_FMT_RGB24, 0, nullptr,
                       nullptr, nullptr),
        sws_freeContext};
    if (!out_convert_ctx) {
      throw std::runtime_error("could not get image conversion context");
    }

    uint8_t *outData[1] = {rgb_frame.data()};
    int outLineSize[1] = {3 * width};

    if (sws_scale(out_convert_ctx.get(),
                  reinterpret_cast<const uint8_t *const *>(frame.planes),
                  frame.stride, 0, height, outData,
                  outLineSize) < 0) {
      throw std::runtime_error("YUV to RGB conversion failed");
    }

    if (view_counter >= output_name_count) {
      throw std::invalid_argument("file mask cannot represent frame " +
                                  std::to_string(view_counter));
    }

    std::string filename = output_names[view_counter];
    view_counter++;

    const std::filesystem::path output_path = filename;
    if (!output_path.parent_path().empty()) {
      std::filesystem::create_directories(output_path.parent_path());
    }

    PPM ppm =
        PPM::create(output_path, width, height, 255);

    for (size_t pixel = 0; pixel < ppm.width() * ppm.height(); ++pixel) {
      ppm.put(pixel, {rgb_frame[pixel * 3 + 0], rgb_frame[pixel * 3 + 1],
                      rgb_frame[pixel * 3 + 2]});
    }
    ppm.flush();
  };

  while (true) {
    size_t size{};
    uint8_t nal_size[4]{};

    input.read(reinterpret_cast<char *>(nal_size), 4);
    if (input.gcount() == 0 && input.eof()) {
      break;
    }
    if (input.gcount() != 4) {
      throw std::runtime_error("unable to read NAL size");
    }

    size = static_cast<uint32_t>(nal_size[0]) |
           (static_cast<uint32_t>(nal_size[1]) << 8) |
           (static_cast<uint32_t>(nal_size[2]) << 16) |
           (static_cast<uint32_t>(nal_size[3]) << 24);
    if (size == 0) {
      throw std::runtime_error("invalid zero-length NAL");
    }

    nal_buffer.clear();
    std::array<uint8_t, 64 * 1024> buffer;
    size_t remaining = size;
    try {
      while (remaining != 0) {
        const size_t chunk = std::min(remaining, buffer.size());
        input.read(reinterpret_cast<char *>(buffer.data()), chunk);
        if (static_cast<size_t>(input.gcount()) != chunk) {
          throw std::runtime_error("unable to read NAL");
        }
        nal_buffer.insert(nal_buffer.end(), buffer.begin(),
                          buffer.begin() + chunk);
        remaining -= chunk;
      }
    } catch (const std::bad_alloc &) {
      throw std::runtime_error("NAL is too large for memory");
    }

    decode(xvc_api, decoder.get(), nal_buffer, saveFrame);
  }

  flush(xvc_api, decoder.get(), saveFrame);

  if (view_counter == 0) {
    throw std::runtime_error("xvc stream contained no pictures");
  }

  return 0;
}
