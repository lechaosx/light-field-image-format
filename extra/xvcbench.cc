/******************************************************************************\
* SOUBOR: av1bench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <xvcenc.h>

extern "C" {
  #include <libswscale/swscale.h>
}

#include <getopt.h>
#include <fstream>
#include <iostream>
#include <sstream>

using std::cerr;
using std::endl;
using std::ofstream;
using std::stringstream;
using std::vector;

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> -o <output-file-name> [-f <first-qp>] [-l <last-qp>] [-a]" << endl;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask {};
  const char *output_file     {};
  const char *first_qp          {};
  const char *last_qp           {};

  vector<uint8_t> rgb_data  {};

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  uint64_t image_count {};

  size_t image_pixels  {};

  int qp_first {30};
  int qp_last  {30};

  bool append     {};

  char opt {};
  while ((opt = getopt(argc, argv, "hi:o:f:l:a")) >= 0) {
    switch (opt) {
      case 'h':
        print_usage(argv[0]);
        return 0;

      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
      break;

      case 'o':
        if (!output_file) {
          output_file = optarg;
          continue;
        }
      break;

      case 'f':
        if (!first_qp) {
          first_qp = optarg;
          continue;
        }
      break;

      case 'l':
        if (!last_qp) {
          last_qp = optarg;
          continue;
        }
      break;

      case 'a':
        if (!append) {
          append = true;
          continue;
        }
      break;

      default:
        print_usage(argv[0]);
        return 1;
      break;
    }
  }

  if (!input_file_mask || !output_file) {
    print_usage(argv[0]);
    return 1;
  }

  if (first_qp) {
    stringstream value {first_qp};
    if (!(value >> qp_first) || !value.eof()) {
      print_usage(argv[0]);
      return 1;
    }
  }

  if (last_qp) {
    stringstream value {last_qp};
    if (!(value >> qp_last) || !value.eof()) {
      print_usage(argv[0]);
      return 1;
    }
  }

  if (qp_first < 0 || qp_last > 63 || qp_first > qp_last) {
    print_usage(argv[0]);
    return 1;
  }

  if (loadPPMGrid(input_file_mask, width, height, color_depth, image_count, rgb_data) < 0) {
    return 2;
  }

  image_pixels = width * height * image_count;

  //////////////////////////////////////////////////////////////////////////////

  ofstream output {};
  if (append) {
    output.open(output_file, std::fstream::app);
  }
  else {
    output.open(output_file, std::fstream::trunc);
    output << "'xvc QP' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
  }
  if (!output) {
    cerr << "Could not open " << output_file << " for writing" << endl;
    return 1;
  }

  const xvc_encoder_api  *xvc_api {};
  xvc_encoder_parameters *params  {};
  xvc_api = xvc_encoder_api_get();
  params = xvc_api->parameters_create();

  xvc_api->parameters_set_default(params);

  params->width = width;
  params->height = height;
  params->chroma_format = XVC_ENC_CHROMA_FORMAT_444;
  params->input_bitdepth = 8;
  params->internal_bitdepth = 8;

  SwsContext *in_convert_ctx = sws_getContext(
    width, height, AV_PIX_FMT_RGB24,
    width, height, AV_PIX_FMT_YUV444P,
    0, nullptr, nullptr, nullptr
  );
  if (!in_convert_ctx) {
    cerr << "Could not get image conversion context" << endl;
    xvc_api->parameters_destroy(params);
    return 1;
  }

  vector<uint8_t> yuv_frame(width * height * 3);

  for (int qp = qp_first; qp <= qp_last; ++qp) {
    params->qp = qp;

    xvc_enc_return_code ret = xvc_api->parameters_check(params);
    if (ret != XVC_ENC_OK) {
      cerr << "xvc parameter error: " << xvc_api->xvc_enc_get_error_text(ret) << endl;
      sws_freeContext(in_convert_ctx);
      xvc_api->parameters_destroy(params);
      return 1;
    }

    xvc_encoder *encoder = xvc_api->encoder_create(params);
    if (!encoder) {
      cerr << "xvc encoder creation failed" << endl;
      sws_freeContext(in_convert_ctx);
      xvc_api->parameters_destroy(params);
      return 1;
    }

    double total_psnr {};
    size_t psnr_values {};
    size_t total_size {};

    auto accountNals = [&](xvc_enc_nal_unit *nal_units, int num_nal_units) {
      for (int i = 0; i < num_nal_units; ++i) {
        total_size += nal_units[i].size;
        if (nal_units[i].stats.psnr_y > 0) {
          total_psnr += nal_units[i].stats.psnr_y;
          total_psnr += nal_units[i].stats.psnr_u;
          total_psnr += nal_units[i].stats.psnr_v;
          psnr_values += 3;
        }
      }
    };

    for (size_t image = 0; image < image_count; ++image) {
      uint8_t *source_data[1] = {&rgb_data[image * width * height * 3]};
      int source_lines[1] = {static_cast<int>(width * 3)};
      uint8_t *destination_data[3] = {
        yuv_frame.data(),
        yuv_frame.data() + width * height,
        yuv_frame.data() + width * height * 2
      };
      int destination_lines[3] = {
        static_cast<int>(width),
        static_cast<int>(width),
        static_cast<int>(width)
      };
      sws_scale(
        in_convert_ctx,
        source_data,
        source_lines,
        0,
        height,
        destination_data,
        destination_lines
      );

      xvc_enc_nal_unit *nal_units {};
      int num_nal_units {};
      ret = xvc_api->encoder_encode(
        encoder,
        yuv_frame.data(),
        &nal_units,
        &num_nal_units,
        nullptr
      );
      if (ret != XVC_ENC_OK) {
        cerr << "xvc encode failed: " << xvc_api->xvc_enc_get_error_text(ret) << endl;
        xvc_api->encoder_destroy(encoder);
        sws_freeContext(in_convert_ctx);
        xvc_api->parameters_destroy(params);
        return 1;
      }

      accountNals(nal_units, num_nal_units);
    }

    while (true) {
      xvc_enc_nal_unit *nal_units {};
      int num_nal_units {};
      ret = xvc_api->encoder_flush(encoder, &nal_units, &num_nal_units, nullptr);
      if (ret != XVC_ENC_OK && ret != XVC_ENC_NO_MORE_OUTPUT) {
        cerr << "xvc flush failed: " << xvc_api->xvc_enc_get_error_text(ret) << endl;
        xvc_api->encoder_destroy(encoder);
        sws_freeContext(in_convert_ctx);
        xvc_api->parameters_destroy(params);
        return 1;
      }

      accountNals(nal_units, num_nal_units);

      if (ret == XVC_ENC_NO_MORE_OUTPUT) {
        break;
      }
    }

    if (psnr_values == 0) {
      cerr << "xvc returned no image metrics" << endl;
      xvc_api->encoder_destroy(encoder);
      sws_freeContext(in_convert_ctx);
      xvc_api->parameters_destroy(params);
      return 1;
    }

    double bpp = total_size * 8.0 / image_pixels;
    double psnr = total_psnr / psnr_values;

    cerr << qp << " " << psnr << " " << bpp << endl;
    output << qp << " " << psnr << " " << bpp << endl;

    xvc_api->encoder_destroy(encoder);
  }

  sws_freeContext(in_convert_ctx);
  xvc_api->parameters_destroy(params);

  output.flush();
  output.close();
  if (!output) {
    cerr << "Could not write " << output_file << endl;
    return 1;
  }

  return 0;
}
