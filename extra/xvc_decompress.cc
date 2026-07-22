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
#include <cmath>

#include <iostream>

using std::cerr;
using std::endl;
using std::ifstream;
using std::string;
using std::vector;
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
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-name> -o <output-file-mask>" << endl;
}

template <typename F>
void decode(const xvc_decoder_api *xvc_api, xvc_decoder *decoder, vector<uint8_t> &nal, F &&callback) {
  xvc_decoded_picture decoded_pic {};

  xvc_dec_return_code ret = xvc_api->decoder_decode_nal(decoder, nal.data(), nal.size(), 0);

  if (ret == XVC_DEC_BITSTREAM_VERSION_LOWER_THAN_SUPPORTED_BY_DECODER) {
    cerr << xvc_api->xvc_dec_get_error_text(ret) << endl;
    exit(XVC_DEC_BITSTREAM_VERSION_LOWER_THAN_SUPPORTED_BY_DECODER);
  } else if (ret == XVC_DEC_BITSTREAM_VERSION_HIGHER_THAN_DECODER) {
    cerr << xvc_api->xvc_dec_get_error_text(ret) << endl;
    exit(XVC_DEC_BITSTREAM_VERSION_HIGHER_THAN_DECODER);
  } else if (ret == XVC_DEC_BITSTREAM_BITDEPTH_TOO_HIGH) {
    cerr << xvc_api->xvc_dec_get_error_text(ret) << endl;
    exit(XVC_DEC_BITSTREAM_BITDEPTH_TOO_HIGH);
  }

  if (xvc_api->decoder_get_picture(decoder, &decoded_pic) == XVC_DEC_OK) {
    callback(decoded_pic);
  }
}

template <typename F>
void flush(const xvc_decoder_api *xvc_api, xvc_decoder *decoder, F &&callback) {
  xvc_decoded_picture decoded_pic {};

  xvc_dec_return_code ret = xvc_api->decoder_flush(decoder);
  if (ret != XVC_DEC_OK) {
    cerr << "Unexpected return code after flush " << ret << endl;
    exit(ret);
  }

  if (xvc_api->decoder_get_picture(decoder, &decoded_pic) == XVC_DEC_OK) {
    callback(decoded_pic);
  }
}

int main(int argc, char *argv[]) {
  const char *input_file_name     {};
  const char *output_file_mask    {};

  const xvc_decoder_api *xvc_api  {};
  xvc_decoder_parameters *params  {};
  xvc_decoder *decoder            {};

  vector<uint8_t> nal_buffer      {};

  ifstream input                   {};

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

  input.open(input_file_name);
  if (!input) {
    cerr << "Could not open " << input_file_name << " for reading\n";
    exit(1);
  }

  size_t view_counter = 0;

  auto saveFrame = [&](xvc_decoded_picture &frame) {
    SwsContext *out_convert_ctx  {};
    vector<uint8_t> rgb_frame {};

    rgb_frame.resize(frame.stats.width * frame.stats.height * 3);

    out_convert_ctx = sws_getContext(frame.stats.width, frame.stats.height, AV_PIX_FMT_YUV444P, frame.stats.width, frame.stats.height, AV_PIX_FMT_RGB24, 0, 0, 0, 0);
    if (!out_convert_ctx) {
      cerr << "Could not get image conversion context" << endl;
      exit(1);
    }

    uint8_t *outData[1]  = { &rgb_frame[0] };
    int outLineSize[1]   = { static_cast<int>(3 * frame.stats.width) };

    sws_scale(out_convert_ctx, reinterpret_cast<const uint8_t *const *>(frame.planes), frame.stride, 0, frame.stats.height, outData, outLineSize);

    std::string filename = get_name_from_mask(output_file_mask, '#', view_counter);

    view_counter++;

    if (create_directory(filename.c_str())) {
      cerr << "ERROR: CANNOT OPEN " << filename << " FOR WRITING\n";
      exit(1);
    }

    PPM ppm {};
    if (ppm.createPPM(filename.c_str(), frame.stats.width, frame.stats.height, 255) < 0) {
      cerr << "ERROR: CANNOT OPEN " << filename << " FOR WRITING" << endl;
      exit(1);
    }

    for (size_t pixel = 0; pixel < ppm.width() * ppm.height(); ++pixel) {
      ppm.put(pixel, {
        rgb_frame[pixel * 3 + 0],
        rgb_frame[pixel * 3 + 1],
        rgb_frame[pixel * 3 + 2]
      });
    }

    sws_freeContext(out_convert_ctx);
  };

  while (true) {
    size_t size    {};
    uint8_t nal_size[4] {};

    input.read(reinterpret_cast<char *>(nal_size), 4);
    if (input.gcount() < 4) {
      break;
    }

    size = nal_size[0] | (nal_size[1] << 8) | (nal_size[2] << 16) | (nal_size[3] << 24);

    nal_buffer.resize(size);
    input.read(reinterpret_cast<char *>(nal_buffer.data()), size);

    if (static_cast<uint32_t>(input.gcount()) < size) {
      cerr << "Unable to read nal." << endl;
      exit(1);
    }

    decode(xvc_api, decoder, nal_buffer, saveFrame);
  }

  flush(xvc_api, decoder, saveFrame);

  xvc_api->decoder_destroy(decoder);
  xvc_api->parameters_destroy(params);

  return 0;
}
