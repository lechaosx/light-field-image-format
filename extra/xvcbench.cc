/******************************************************************************\
* SOUBOR: av1bench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <xvcenc.h>

#include <getopt.h>
#include <cmath>

#include <iostream>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <cassert>
#include <bitset>

using namespace std;

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> -o <output-file-name> [-f <fist-bitrate>] [-l <last-bitrate>] [-a]" << endl;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask {};
  const char *output_file     {};
  const char *first_bitrate     {};
  const char *last_bitrate     {};

  vector<uint8_t> rgb_data  {};

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  uint64_t image_count {};

  size_t image_pixels  {};

  double f_b {};
  double l_b {};

  bool append     {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:o:f:l:a")) >= 0) {
    switch (opt) {
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
        if (!first_bitrate) {
          first_bitrate = optarg;
          continue;
        }
      break;

      case 'l':
        if (!last_bitrate) {
          last_bitrate = optarg;
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

  f_b = 0.1;
  if (first_bitrate) {
    stringstream(first_bitrate) >> l_b;
  }

  l_b = 15;
  if (last_bitrate) {
    stringstream(last_bitrate) >> l_b;
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
    output << "'xvc' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
  }

  const xvc_encoder_api  *xvc_api {};
  xvc_encoder_parameters *params  {};
  xvc_encoder            *encoder {};
  xvc_enc_pic_buffer rec_pic_buffer = { 0, 0 };

  xvc_api = xvc_encoder_api_get();
  params = xvc_api->parameters_create();

  xvc_api->parameters_set_default(params);

  params->width = width;
  params->height = height;
  params->chroma_format = XVC_ENC_CHROMA_FORMAT_444;
  params->input_bitdepth = 8;

  xvc_enc_return_code ret = xvc_api->parameters_check(params);
  if (ret != XVC_ENC_OK) {
    cerr << xvc_api->xvc_enc_get_error_text(ret) << endl;
    return 0;
  }

  encoder = xvc_api->encoder_create(params);

  xvc_enc_nal_unit *nal_units;
  int num_nal_units;

  bool   loop_check {true};
  size_t img        {0};
  size_t rec_ite    {0};
  double mse        {0.0};
  size_t total_size {0};
  while (loop_check) {
    if (img < image_count) {
      cerr << "IMG: " << img << "\n";
      xvc_enc_return_code ret = xvc_api->encoder_encode(encoder, &rgb_data[img * width * height * 3], &nal_units, &num_nal_units, &rec_pic_buffer);
      assert(ret == XVC_ENC_OK || ret == XVC_ENC_NO_MORE_OUTPUT);
      img++;
    }
    else {
      xvc_enc_return_code ret = xvc_api->encoder_flush(encoder, &nal_units, &num_nal_units, &rec_pic_buffer);
      loop_check = (ret == XVC_ENC_OK);
    }

    for (int i = 0; i < num_nal_units; i++) {
      cerr << "NAL: " << i << "\n";
      total_size += nal_units[i].size;
    }

    if (rec_pic_buffer.size > 0) {
      for (size_t i = 0; i < image_pixels * 3; i++) {
        cerr << bitset<8>(rgb_data[rec_ite]) << " " << bitset<8>(rec_pic_buffer.pic[i]) << "\n";
        double tmp = rgb_data[rec_ite] - rec_pic_buffer.pic[i];
        mse += tmp * tmp;
        rec_ite++;
      }
    }
  }

  mse /= image_count * width * height * 3;

  double bpp = total_size * 8.0 / image_pixels;
  double psnr = 10 * log10((255 * 255) / mse);

  cerr   << psnr << " " << bpp << endl;
  output << psnr << " " << bpp << endl;

  if (encoder) {
    xvc_api->encoder_destroy(encoder);
  }
  if (params) {
    xvc_api->parameters_destroy(params);
  }

  return 0;
}
