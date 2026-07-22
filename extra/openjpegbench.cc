/******************************************************************************\
* SOUBOR: openjpegbench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <getopt.h>

#include <cmath>

#include <iostream>
#include <fstream>
#include <vector>

#include <openjpeg.h>

using namespace std;

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> -o <output-file-name> [-f <fist-psnr>] [-l <last-psnr>] [-s <psnr-step>] [-a]" << endl;
}

static void error_callback(const char *msg, void *) {
  cerr << "[ERROR] " << msg;
}

static void warning_callback(const char *msg, void *) {
  cerr << "[WARNING] " << msg;
}

static void info_callback(const char *, void *) {
}

int main(int argc, char *argv[]) {
  const char *input_file_mask {};
  const char *output_file     {};
  const char *param_psnr_first   {};
  const char *param_psnr_last    {};

  const char *param_psnr_step    {};

  float psnr_step  {};
  float psnr_first {};
  float psnr_last  {};
  bool append     {};

  vector<uint8_t> rgb_data {};

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  uint64_t image_count {};

  opj_cparameters_t    cparameters {};
  opj_dparameters_t    dparameters {};
  opj_image_cmptparm_t cmptparm[3] {};
  opj_image_t         *l_image       {};
  opj_codec_t         *l_codec     {};
  opj_stream_t        *l_stream    {};

  char opt {};
  while ((opt = getopt(argc, argv, "hi:o:s:f:l:a")) >= 0) {
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

      case 's':
        if (!param_psnr_step) {
          param_psnr_step = optarg;
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
        if (!param_psnr_first) {
          param_psnr_first = optarg;
          continue;
        }
      break;

      case 'l':
        if (!param_psnr_last) {
          param_psnr_last = optarg;
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

  psnr_step = 1.0;

  if (param_psnr_step) {
    psnr_step = atof(param_psnr_step);
  }

  psnr_first = 20.f;
  if (param_psnr_first) {
    psnr_first = atof(param_psnr_first);
  }

  psnr_last = 50.f;
  if (param_psnr_last) {
    psnr_last = atof(param_psnr_last);
  }

  if (loadPPMGrid(input_file_mask, width, height, color_depth, image_count, rgb_data) < 0) {
    return 2;
  }

  ///////////////////////////////////

  opj_set_default_encoder_parameters(&cparameters);
  cparameters.cod_format = OPJ_CODEC_J2K;
  cparameters.tcp_numlayers = 1;
  cparameters.cp_fixed_quality = 1;
  cparameters.irreversible = 1;

  opj_set_default_decoder_parameters(&dparameters);
  dparameters.decod_format = OPJ_CODEC_J2K;
  dparameters.cp_layer = 0;
  dparameters.cp_reduce = 0;

  for (size_t i = 0; i < 3; i++) {
    cmptparm[i].dx   = 1;
    cmptparm[i].dy   = 1;
    cmptparm[i].w    = width;
    cmptparm[i].h    = height;
    cmptparm[i].x0   = 0;
    cmptparm[i].y0   = 0;
    cmptparm[i].prec = 8;
    cmptparm[i].bpp  = 8;
    cmptparm[i].sgnd = 0;
  }

  ofstream output {};
  if (append) {
    output.open(output_file, std::fstream::app);
  }
  else {
    output.open(output_file, std::fstream::trunc);
    output << "'openjpeg' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
  }

  size_t image_pixels = width * height * image_count;

  for (size_t param_psnr = psnr_first; param_psnr <= psnr_last; param_psnr += psnr_step) {
    cerr << "PSNR: " << param_psnr << "\n";

    double mse = 0;
    size_t compressed_size = 0;

    cparameters.tcp_distoratio[0] = param_psnr;

    for (size_t img = 0; img < image_count; img++) {
      l_image = opj_image_create(3, cmptparm, OPJ_CLRSPC_SRGB);
      l_image->x0 = 0;
      l_image->y0 = 0;
      l_image->x1 = width;
      l_image->y1 = height;
      l_image->color_space = OPJ_CLRSPC_SRGB;

      for (size_t pixel = 0; pixel < width * height; pixel++) {
        for(size_t component = 0; component < 3; component++) {
          l_image->comps[component].data[pixel] = rgb_data[(img * width * height + pixel) * 3 + component];
        }
      }

      l_stream = opj_stream_create_default_file_stream("/tmp/openjpeg.j2k", OPJ_FALSE);

      l_codec = opj_create_compress(OPJ_CODEC_J2K);
      opj_set_info_handler(l_codec, info_callback, nullptr);
      opj_set_warning_handler(l_codec, warning_callback, nullptr);
      opj_set_error_handler(l_codec, error_callback, nullptr);

      opj_setup_encoder(l_codec, &cparameters, l_image);
      opj_start_compress(l_codec, l_image, l_stream);
      opj_encode(l_codec, l_stream);
      opj_end_compress(l_codec, l_stream);

      opj_destroy_codec(l_codec);
      opj_stream_destroy(l_stream);
      opj_image_destroy(l_image);

      std::ifstream in("/tmp/openjpeg.j2k", std::ifstream::ate | std::ifstream::binary);
      compressed_size += in.tellg();

      l_stream = opj_stream_create_default_file_stream("/tmp/openjpeg.j2k", OPJ_TRUE);

      l_codec = opj_create_decompress(OPJ_CODEC_J2K);
      opj_set_info_handler(l_codec, info_callback, nullptr);
      opj_set_warning_handler(l_codec, warning_callback, nullptr);
      opj_set_error_handler(l_codec, error_callback, nullptr);

      opj_codec_set_threads(l_codec, 4);

      opj_setup_decoder(l_codec, &dparameters);

      opj_read_header(l_stream, l_codec, &l_image);
      opj_decode(l_codec, l_stream, l_image);
      opj_end_decompress(l_codec,l_stream);

      for (size_t pixel = 0; pixel < width * height; pixel++) {
        for(size_t component = 0; component < 3; component++) {
          double tmp = rgb_data[(img * width * height + pixel) * 3 + component] - l_image->comps[component].data[pixel];
          mse += tmp * tmp;
        }
      }

      opj_stream_destroy(l_stream);
      opj_destroy_codec(l_codec);
      opj_image_destroy(l_image);
    }

    mse /= image_count * width * height * 3;

    double bpp = compressed_size * 8.0 / image_pixels;
    double psnr = 10 * log10((255 * 255) / mse);

    cerr << param_psnr  << " " << psnr << " " << bpp << endl;
    output << param_psnr  << " " << psnr << " " << bpp << endl;
  }


  return 0;
}
