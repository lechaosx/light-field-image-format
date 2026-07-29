/******************************************************************************\
* SOUBOR: mozbench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"


#include <getopt.h>

#include <charconv>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>


#include <jpeglib.h>

void print_usage(char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0 << " -i <input-file-mask> -o <output-file-name> [-f <fist-quality>] [-l <last-quality>] [-s <quality-step>] [-a]" << std::endl;
}

uint8_t parseQuality(std::string_view input) {
  unsigned value {};
  const auto result = std::from_chars(input.data(), input.data() + input.size(), value);
  if (result.ec != std::errc {} || result.ptr != input.data() + input.size()
      || value < 1 || value > 100) {
    throw std::invalid_argument("quality must be an integer from 1 to 100");
  }
  return static_cast<uint8_t>(value);
}

void run(int argc, char *argv[]) {
  const char *input_file_mask {};
  const char *output_file  {};
  const char *quality_first {};
  const char *quality_last {};

  const char *quality_step    {};

  uint8_t q_step  {};
  uint8_t q_first  {};
  uint8_t q_last  {};
  bool append             {};

  std::vector<PPM> images       {};

  uint64_t width       {};
  uint64_t height      {};
  uint32_t color_depth {};
  uint64_t image_count {};

  jpeg_compress_struct cinfo   {};
  jpeg_decompress_struct dinfo {};
  jpeg_error_mgr jerr          {};
  JSAMPROW row_pointer[1]      {};
  int row_stride               {};

  char opt {};
  while ((opt = getopt(argc, argv, "hi:o:s:f:l:a")) >= 0) {
    switch (opt) {
      case 'h':
        print_usage(argv[0]);
        return;

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

      case 'o':
        if (!output_file) {
          output_file = optarg;
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

      case 'a':
        if (!append) {
          append = true;
          continue;
        }
      break;

      default:
        print_usage(argv[0]);
        throw std::invalid_argument("invalid arguments");
      break;
    }
  }

  if (!input_file_mask || !output_file) {
    print_usage(argv[0]);
    throw std::invalid_argument("input and output are required");
  }

  q_step = 1;

  if (quality_step) {
    q_step = parseQuality(quality_step);
  }

  q_first = q_step;
  if (quality_first) {
    q_first = parseQuality(quality_first);
  }

  q_last = 100;
  if (quality_last) {
    q_last = parseQuality(quality_last);
  }
  if (q_first > q_last) {
    throw std::invalid_argument("qualities must be ordered from first to last");
  }

  images = mapPPMs(input_file_mask);
  width = images.front().width();
  height = images.front().height();
  color_depth = images.front().color_depth();
  image_count = images.size();
  const uint64_t view_side = std::sqrt(image_count);
  if (view_side * view_side != image_count || color_depth > 255) {
    throw std::invalid_argument("input must be a square grid of 8-bit PPM images");
  }

  cinfo.err = jpeg_std_error(&jerr);
  dinfo.err = jpeg_std_error(&jerr);

  jpeg_create_compress(&cinfo);
  jpeg_create_decompress(&dinfo);

  try {
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);

    row_stride = width * 3;

    std::ofstream output {};
    if (append) {
      output.open(output_file, std::fstream::app);
    }
    else {
      output.open(output_file, std::fstream::trunc);
      output << "'mozjpeg' 'PSNR [dB]' 'bitrate [bpp]'" << std::endl;
    }
    if (!output) {
      throw std::ios_base::failure(
          "could not open " + std::string(output_file) + " for writing");
    }

    size_t image_pixels = width * height * image_count;

    for (size_t quality = q_first; quality <= q_last; quality += q_step) {
      std::cerr << "Q" << quality << " STARTED" << std::endl;

      size_t compressed_size = 0;
      double mse = 0;

      jpeg_set_quality(&cinfo, quality, TRUE);

      cinfo.comp_info[0].h_samp_factor = 1;
      cinfo.comp_info[0].v_samp_factor = 1;
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;

      for (size_t i = 0; i < image_count; i++) {
        const auto *original = images[i].pixels().data();
        std::vector<uint8_t>  decompressed_rgb_data(width * height * 3);

        auto close_file = [](FILE *file) { fclose(file); };
        std::unique_ptr<FILE, decltype(close_file)> compressed(std::tmpfile(), close_file);
        if (!compressed) {
          throw std::runtime_error("could not create temporary JPEG file");
        }

        jpeg_stdio_dest(&cinfo, compressed.get());

        jpeg_start_compress(&cinfo, TRUE);

        while (cinfo.next_scanline < cinfo.image_height) {
          row_pointer[0] =
              const_cast<JSAMPROW>(&original[cinfo.next_scanline * row_stride]);
          jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);

        const long jpeg_size = ftell(compressed.get());
        if (jpeg_size < 0) {
          throw std::runtime_error("could not determine temporary JPEG size");
        }
        compressed_size += static_cast<size_t>(jpeg_size);
        rewind(compressed.get());

        jpeg_stdio_src(&dinfo, compressed.get());

        jpeg_read_header(&dinfo, TRUE);

        jpeg_start_decompress(&dinfo);

        while (dinfo.output_scanline < dinfo.output_height) {
          row_pointer[0] = &decompressed_rgb_data[dinfo.output_scanline * row_stride];
          jpeg_read_scanlines(&dinfo, row_pointer, 1);
        }

        jpeg_finish_decompress(&dinfo);

        for (size_t pix = 0; pix < decompressed_rgb_data.size(); pix++) {
          double tmp = original[pix] - decompressed_rgb_data[pix];
          mse += tmp * tmp;
        }
      }

      mse /= image_count * width * height * 3;

      double bpp = compressed_size * 8.0 / image_pixels;
      double psnr = 10 * log10((255 * 255) / mse);

      std::cerr << quality  << " " << psnr << " " << bpp << std::endl;
      output << quality  << " " << psnr << " " << bpp << std::endl;
    }

    output.flush();
    output.close();
    if (!output) {
      throw std::ios_base::failure("could not write " + std::string(output_file));
    }
  } catch (...) {
    jpeg_destroy_compress(&cinfo);
    jpeg_destroy_decompress(&dinfo);
    throw;
  }

  jpeg_destroy_compress(&cinfo);
  jpeg_destroy_decompress(&dinfo);
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
