/******************************************************************************\
* SOUBOR: lfifbench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <lfiflib.h>

#include <getopt.h>

#include <cmath>

#include <thread>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> [-2 <output-file-name>] [-3 <output-file-name>] [-4 <output-file-name>] [-f <fist-quality>] [-l <last-quality>] [-s <quality-step>] [-a]" << endl;
}

template<typename T>
double MSE(const T *cmp1, const T *cmp2, size_t size) {
  double mse = 0;
  for (size_t i = 0; i < size; i++) {
    mse += (cmp1[i] - cmp2[i]) * (cmp1[i] - cmp2[i]);
  }
  return mse / size;
}

double PSNR(double mse, size_t max) {
  if (mse == .0) {
    return 0;
  }
  return 10 * log10((max * max) / mse);
}

size_t fileSize(const char *filename) {
  ifstream encoded_file(filename, ifstream::ate | ifstream::binary);
  return encoded_file.tellg();
}

int doTest(LFIFCompressStruct cinfo, const vector<uint8_t> &original, ofstream &output, size_t q_first, size_t q_last, size_t q_step) {
  LFIFDecompressStruct dinfo   {};
  size_t image_pixels          {};
  size_t compressed_image_size {};
  int    errcode               {};
  double psnr                  {};
  double bpp                   {};
  vector<uint8_t> decompressed {};

  dinfo.image_width     = cinfo.image_width;
  dinfo.image_height    = cinfo.image_height;
  dinfo.image_count     = cinfo.image_count;
  dinfo.max_rgb_value   = cinfo.max_rgb_value;
  dinfo.method          = cinfo.method;
  dinfo.input_file_name = cinfo.output_file_name;

  image_pixels = cinfo.image_width * cinfo.image_height * cinfo.image_count;

  decompressed.resize(original.size());

  for (size_t quality = q_first; quality <= q_last; quality += q_step) {
    cinfo.quality = quality;

    errcode = LFIFCompress(&cinfo, original.data());

    if (errcode) {
      cerr << "ERROR: UNABLE TO OPEN FILE \"" << cinfo.output_file_name << "\" FOR WRITITNG" << endl;
      return -1;
    }

    compressed_image_size = fileSize(cinfo.output_file_name);
    errcode = LFIFDecompress(&dinfo, decompressed.data());

    if (errcode) {
      switch (errcode) {
        case -1:
          cerr << "ERROR: UNABLE TO OPEN FILE \"" << dinfo.input_file_name << "\" FOR READING" << endl;
        break;

        case -2:
          cerr << "ERROR: MAGIC NUMBER MISMATCH" << endl;
        break;
      }

      return -1;
    }

    if (dinfo.max_rgb_value < 256) {
      psnr = PSNR(MSE<uint8_t>(original.data(), decompressed.data(), image_pixels * 3), dinfo.max_rgb_value);
    }
    else {
      psnr = PSNR(MSE<uint16_t>(reinterpret_cast<const uint16_t *>(original.data()), reinterpret_cast<const uint16_t *>(decompressed.data()), image_pixels * 3), dinfo.max_rgb_value);
    }

    bpp = compressed_image_size * 8.0 / image_pixels;

    output << quality  << " " << psnr << " " << bpp << endl;
  }

  return 0;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask {};
  const char *output_file_2D  {};
  const char *output_file_3D  {};
  const char *output_file_4D  {};
  const char *quality_step    {};
  const char *quality_first   {};
  const char *quality_last    {};

  bool nothreads              {};
  bool append                 {};
  uint8_t q_step              {};
  uint8_t q_first             {};
  uint8_t q_last              {};

  vector<uint8_t> rgb_data    {};

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  uint64_t image_count {};

  LFIFCompressStruct   cinfo {};

  ofstream outputs[3] {};

  vector<thread> threads {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:s:f:l:2:3:4:na")) >= 0) {
    switch (opt) {
      case 'i':
        if (!input_file_mask) {
          input_file_mask = optarg;
          continue;
        }
      break;

      case 's':
        if (!quality_step) {
          quality_step = optarg;
          continue;
        }
      break;

      case 'f':
        if (!quality_first) {
          quality_first = optarg;
          continue;
        }
      break;

      case 'l':
        if (!quality_last) {
          quality_last = optarg;
          continue;
        }
      break;

      case '2':
        if (!output_file_2D) {
          output_file_2D = optarg;
          continue;
        }
      break;

      case '3':
        if (!output_file_3D) {
          output_file_3D = optarg;
          continue;
        }
      break;

      case '4':
        if (!output_file_4D) {
          output_file_4D = optarg;
          continue;
        }
      break;

      case 'n':
        if (!nothreads) {
          nothreads = true;
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

  if (!input_file_mask) {
    print_usage(argv[0]);
    return 1;
  }

  if (!output_file_2D && !output_file_3D && !output_file_4D) {
    cerr << "Please specify one or more options [-2 <output-filename>] [-3 <output-filename>] [-4 <output-filename>]." << endl;
    print_usage(argv[0]);
    return 1;
  }

  q_step = 1;
  if (quality_step) {
    int tmp = atoi(quality_step);
    if ((tmp < 1) || (tmp > 100)) {
      print_usage(argv[0]);
      return 1;
    }
    q_step = tmp;
  }

  q_first = q_step;
  if (quality_first) {
    int tmp = atoi(quality_first);
    if ((tmp < 1) || (tmp > 100)) {
      print_usage(argv[0]);
      return 1;
    }
    q_first = tmp;
  }

  q_last = 100;
  if (quality_last) {
    int tmp = atoi(quality_last);
    if ((tmp < 1) || (tmp > 100)) {
      print_usage(argv[0]);
      return 1;
    }
    q_last = tmp;
  }

  if (!checkPPMheaders(input_file_mask, width, height, color_depth, image_count)) {
    return 2;
  }

  if (color_depth < 256) {
    rgb_data.resize(width * height * image_count * 3);
  }
  else {
    rgb_data.resize(width * height * image_count * 3 * 2);
  }

  if (!loadPPMs(input_file_mask, rgb_data.data())) {
    return 3;
  }

  cinfo.image_width      = width;
  cinfo.image_height     = height;
  cinfo.image_count      = image_count;
  cinfo.max_rgb_value    = color_depth;

  if (output_file_2D) {
    cinfo.method = LFIF_2D;
    cinfo.output_file_name = "/tmp/lfifbench.lfif2d";

    if (append) {
      outputs[0].open(output_file_2D, std::fstream::app);
    }
    else {
      outputs[0].open(output_file_2D, std::fstream::trunc);
      outputs[0] << "'2D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
    }

    if (nothreads) {
      doTest(cinfo, rgb_data, outputs[0], q_first, q_last, q_step);
    }
    else {
      threads.push_back(thread(doTest, cinfo, ref(rgb_data), ref(outputs[0]), q_first, q_last, q_step));
    }
  }

  if (output_file_3D) {
    cinfo.method = LFIF_3D;
    cinfo.output_file_name = "/tmp/lfifbench.lfif3d";

    if (append) {
      outputs[1].open(output_file_3D, std::fstream::app);
    }
    else {
      outputs[1].open(output_file_3D, std::fstream::trunc);
      outputs[1] << "'3D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
    }

    if (nothreads) {
      doTest(cinfo, rgb_data, outputs[1], q_first, q_last, q_step);
    }
    else {
      threads.push_back(thread(doTest, cinfo, ref(rgb_data), ref(outputs[1]), q_first, q_last, q_step));
    }
  }

  if (output_file_4D) {
    cinfo.method = LFIF_4D;
    cinfo.output_file_name = "/tmp/lfifbench.lfif4d";

    if (append) {
      outputs[2].open(output_file_4D, std::fstream::app);
    }
    else {
      outputs[2].open(output_file_4D, std::fstream::trunc);
      outputs[2] << "'4D' 'PSNR [dB]' 'bitrate [bpp]'" << endl;
    }

    if (nothreads) {
      doTest(cinfo, rgb_data, outputs[2], q_first, q_last, q_step);
    }
    else {
      threads.push_back(thread(doTest, cinfo, ref(rgb_data), ref(outputs[2]), q_first, q_last, q_step));
    }
  }

  for (auto &thread: threads) {
    thread.join();
  }

  return 0;
}