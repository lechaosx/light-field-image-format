/******************************************************************************\
* SOUBOR: av1bench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "ffmpeg_raii.h"
#include "plenoppm.h"

#include <xvcenc.h>

#include <cmath>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

void print_usage(char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0
            << " -i <input-file-mask> -o <output-file-name> [-f <first-qp>] "
               "[-l <last-qp>] [-a]"
            << std::endl;
}

int main(int argc, char *argv[]) {
  const char *input_file_mask{};
  const char *output_file{};
  const char *first_qp{};
  const char *last_qp{};

  std::vector<PPM> images{};

  int width{};
  int height{};
  uint32_t color_depth{};
  size_t image_count{};

  size_t image_pixels{};

  int qp_first{30};
  int qp_last{30};

  bool append{};

  char opt{};
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
      throw std::invalid_argument("invalid arguments");
      break;
    }
  }

  if (!input_file_mask || !output_file) {
    print_usage(argv[0]);
    throw std::invalid_argument("input and output are required");
  }

  if (first_qp) {
    std::stringstream value{first_qp};
    if (!(value >> qp_first) || !value.eof()) {
      print_usage(argv[0]);
      throw std::invalid_argument("first QP must be an integer");
    }
  }

  if (last_qp) {
    std::stringstream value{last_qp};
    if (!(value >> qp_last) || !value.eof()) {
      print_usage(argv[0]);
      throw std::invalid_argument("last QP must be an integer");
    }
  }

  if (qp_first < 0 || qp_last > 63 || qp_first > qp_last) {
    print_usage(argv[0]);
    throw std::invalid_argument("QP range must be ordered within 0 to 63");
  }

  images = mapPPMs(input_file_mask);
  const uint64_t input_width = images.front().width();
  const uint64_t input_height = images.front().height();
  if (input_width > static_cast<uint64_t>(std::numeric_limits<int>::max()) / 3
      || input_height > static_cast<uint64_t>(std::numeric_limits<int>::max())) {
    throw std::invalid_argument("PPM dimensions exceed codec limits");
  }
  width = static_cast<int>(input_width);
  height = static_cast<int>(input_height);
  color_depth = images.front().color_depth();
  image_count = images.size();
  const size_t view_side = std::sqrt(image_count);
  if (view_side * view_side != image_count || color_depth > 255) {
    throw std::invalid_argument(
        "input must be a square grid of 8-bit PPM images");
  }

  const size_t frame_pixels = static_cast<size_t>(width) * height;
  if (image_count > std::numeric_limits<size_t>::max() / 3
      || frame_pixels > std::numeric_limits<size_t>::max() / (image_count * 3)) {
    throw std::length_error("PPM grid is too large");
  }
  image_pixels = frame_pixels * image_count;

  std::ofstream output{};
  if (append) {
    output.open(output_file, std::fstream::app);
  } else {
    output.open(output_file, std::fstream::trunc);
    output << "'xvc QP' 'PSNR [dB]' 'bitrate [bpp]'" << std::endl;
  }
  if (!output) {
    throw std::ios_base::failure("could not open " + std::string(output_file) +
                                 " for writing");
  }

  const xvc_encoder_api *xvc_api = xvc_encoder_api_get();
  const auto delete_parameters =
      [xvc_api](xvc_encoder_parameters *parameters) noexcept {
        xvc_api->parameters_destroy(parameters);
      };
  std::unique_ptr<xvc_encoder_parameters, decltype(delete_parameters)> params{
      xvc_api->parameters_create(), delete_parameters};
  if (!params) {
    throw std::runtime_error("xvc parameter creation failed");
  }

  xvc_api->parameters_set_default(params.get());

  params->width = width;
  params->height = height;
  params->chroma_format = XVC_ENC_CHROMA_FORMAT_444;
  params->input_bitdepth = 8;
  params->internal_bitdepth = 8;

  ffmpeg::ScaleContext in_convert_ctx{
      sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height,
                     AV_PIX_FMT_YUV444P, 0, nullptr, nullptr, nullptr),
      sws_freeContext};
  if (!in_convert_ctx) {
    throw std::runtime_error("could not get image conversion context");
  }

  std::vector<uint8_t> yuv_frame(frame_pixels * 3);

  for (int qp = qp_first; qp <= qp_last; ++qp) {
    params->qp = qp;

    xvc_enc_return_code ret = xvc_api->parameters_check(params.get());
    if (ret != XVC_ENC_OK) {
      throw std::invalid_argument(std::string("xvc parameter error: ") +
                                  xvc_api->xvc_enc_get_error_text(ret));
    }

    const auto delete_encoder = [xvc_api](xvc_encoder *encoder) noexcept {
      xvc_api->encoder_destroy(encoder);
    };
    std::unique_ptr<xvc_encoder, decltype(delete_encoder)> encoder{
        xvc_api->encoder_create(params.get()), delete_encoder};
    if (!encoder) {
      throw std::runtime_error("xvc encoder creation failed");
    }

    double total_psnr{};
    size_t psnr_values{};
    size_t total_size{};

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
      const uint8_t *source_data[1] = {images[image].pixels().data()};
      int source_lines[1] = {width * 3};
      uint8_t *destination_data[3] = {yuv_frame.data(),
                                      yuv_frame.data() + frame_pixels,
                                      yuv_frame.data() + frame_pixels * 2};
      int destination_lines[3] = {width, width, width};
      if (sws_scale(in_convert_ctx.get(), source_data, source_lines, 0, height,
                    destination_data, destination_lines) < 0) {
        throw std::runtime_error("RGB to YUV conversion failed");
      }

      xvc_enc_nal_unit *nal_units{};
      int num_nal_units{};
      ret = xvc_api->encoder_encode(encoder.get(), yuv_frame.data(), &nal_units,
                                    &num_nal_units, nullptr);
      if (ret != XVC_ENC_OK) {
        throw std::runtime_error(std::string("xvc encode failed: ") +
                                 xvc_api->xvc_enc_get_error_text(ret));
      }

      accountNals(nal_units, num_nal_units);
    }

    while (true) {
      xvc_enc_nal_unit *nal_units{};
      int num_nal_units{};
      ret = xvc_api->encoder_flush(encoder.get(), &nal_units, &num_nal_units,
                                   nullptr);
      if (ret != XVC_ENC_OK && ret != XVC_ENC_NO_MORE_OUTPUT) {
        throw std::runtime_error(std::string("xvc flush failed: ") +
                                 xvc_api->xvc_enc_get_error_text(ret));
      }

      accountNals(nal_units, num_nal_units);

      if (ret == XVC_ENC_NO_MORE_OUTPUT) {
        break;
      }
    }

    if (psnr_values == 0) {
      throw std::runtime_error("xvc returned no image metrics");
    }

    double bpp = total_size * 8.0 / image_pixels;
    double psnr = total_psnr / psnr_values;

    std::cerr << qp << " " << psnr << " " << bpp << std::endl;
    output << qp << " " << psnr << " " << bpp << std::endl;
  }

  output.flush();
  output.close();
  if (!output) {
    throw std::ios_base::failure("could not write " + std::string(output_file));
  }

  return 0;
}
