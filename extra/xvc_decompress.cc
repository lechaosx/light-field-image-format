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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>

void print_usage(char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0 << " -i <input-file-name> -o <output-file-mask>" << std::endl;
}

template <typename F>
void outputPictures(const xvc_decoder_api *xvc_api, xvc_decoder *decoder, F &&callback) {
  xvc_decoded_picture decoded_pic {};

  while (true) {
    const xvc_dec_return_code ret = xvc_api->decoder_get_picture(decoder, &decoded_pic);
    if (ret == XVC_DEC_NO_DECODED_PIC) {
      return;
    }
    if (ret != XVC_DEC_OK) {
      throw std::runtime_error(
          std::string("xvc output failed: ") + xvc_api->xvc_dec_get_error_text(ret));
    }
    callback(decoded_pic);
  }
}

template <typename F>
void decode(
    const xvc_decoder_api *xvc_api,
    xvc_decoder *decoder,
    std::vector<uint8_t> &nal,
    F &&callback) {
  const xvc_dec_return_code ret = xvc_api->decoder_decode_nal(decoder, nal.data(), nal.size(), 0);
  if (ret != XVC_DEC_OK) {
    throw std::runtime_error(
        std::string("xvc decode failed: ") + xvc_api->xvc_dec_get_error_text(ret));
  }
  outputPictures(xvc_api, decoder, callback);
}

template <typename F>
void flush(const xvc_decoder_api *xvc_api, xvc_decoder *decoder, F &&callback) {
  const xvc_dec_return_code ret = xvc_api->decoder_flush(decoder);
  if (ret != XVC_DEC_OK) {
    throw std::runtime_error(
        std::string("xvc flush failed: ") + xvc_api->xvc_dec_get_error_text(ret));
  }
  outputPictures(xvc_api, decoder, callback);
}

void run(int argc, char *argv[]) {
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
        return;

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

  xvc_api = xvc_decoder_api_get();
  params  = xvc_api->parameters_create();
  if (!params) {
    throw std::runtime_error("xvc parameter creation failed");
  }

  try {
    xvc_api->parameters_set_default(params);
    const xvc_dec_return_code parameter_result = xvc_api->parameters_check(params);
    if (parameter_result != XVC_DEC_OK) {
      throw std::runtime_error(
          std::string("xvc parameter error: ")
          + xvc_api->xvc_dec_get_error_text(parameter_result));
    }

    decoder = xvc_api->decoder_create(params);
    if (!decoder) {
      throw std::runtime_error("xvc decoder creation failed");
    }

    input.open(input_file_name, std::ios::binary);
    if (!input) {
      throw std::ios_base::failure(
          "could not open " + std::string(input_file_name) + " for reading");
    }

    size_t view_counter = 0;
    const size_t output_name_count = get_mask_names_count(output_file_mask, '#');
    if (output_name_count == 0) {
      throw std::invalid_argument("output file mask is too large");
    }

    auto saveFrame = [&](xvc_decoded_picture &frame) {
      SwsContext *out_convert_ctx {};
      std::vector<uint8_t> rgb_frame(frame.stats.width * frame.stats.height * 3);

      out_convert_ctx = sws_getContext(frame.stats.width, frame.stats.height, AV_PIX_FMT_YUV444P, frame.stats.width, frame.stats.height, AV_PIX_FMT_RGB24, 0, 0, 0, 0);
      if (!out_convert_ctx) {
        throw std::runtime_error("could not get image conversion context");
      }

      try {
        uint8_t *outData[1] = {rgb_frame.data()};
        int outLineSize[1] = {static_cast<int>(3 * frame.stats.width)};

        if (sws_scale(
                out_convert_ctx,
                reinterpret_cast<const uint8_t *const *>(frame.planes),
                frame.stride,
                0,
                frame.stats.height,
                outData,
                outLineSize) < 0) {
          throw std::runtime_error("YUV to RGB conversion failed");
        }

        if (view_counter >= output_name_count) {
          throw std::invalid_argument(
              "file mask cannot represent frame " + std::to_string(view_counter));
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
      } catch (...) {
        sws_freeContext(out_convert_ctx);
        throw;
      }

      sws_freeContext(out_convert_ctx);
    };

    while (true) {
      size_t size {};
      uint8_t nal_size[4] {};

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
          nal_buffer.insert(nal_buffer.end(), buffer.begin(), buffer.begin() + chunk);
          remaining -= chunk;
        }
      } catch (const std::bad_alloc &) {
        throw std::runtime_error("NAL is too large for memory");
      }

      decode(xvc_api, decoder, nal_buffer, saveFrame);
    }

    flush(xvc_api, decoder, saveFrame);

    if (view_counter == 0) {
      throw std::runtime_error("xvc stream contained no pictures");
    }
  } catch (...) {
    if (decoder) {
      xvc_api->decoder_destroy(decoder);
    }
    xvc_api->parameters_destroy(params);
    throw;
  }

  xvc_api->decoder_destroy(decoder);
  xvc_api->parameters_destroy(params);
}

int main(int argc, char *argv[]) {
  try {
    run(argc, argv);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}
