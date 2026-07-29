/******************************************************************************\
* SOUBOR: xvc_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "ffmpeg_raii.h"
#include "plenoppm.h"

#include <xvcenc.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

void print_usage(char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0 << " -i <input-file-mask> -o <output-file-name> -q <qp>"
            << std::endl;
}

template <typename F>
void encode(const xvc_encoder_api *xvc_api, xvc_encoder *encoder,
            const uint8_t *buffer, F &&callback) {
  xvc_enc_pic_buffer *rec_pic_ptr{nullptr};
  xvc_enc_nal_unit *nal_units;
  int num_nal_units;

  xvc_enc_return_code ret = xvc_api->encoder_encode(
      encoder, buffer, &nal_units, &num_nal_units, rec_pic_ptr);
  if (ret != XVC_ENC_OK) {
    throw std::runtime_error(std::string("xvc encode failed: ") +
                             xvc_api->xvc_enc_get_error_text(ret));
  }

  for (int i = 0; i < num_nal_units; i++) {
    callback(nal_units[i]);
  }
}

template <typename F>
void flush(const xvc_encoder_api *xvc_api, xvc_encoder *encoder, F &&callback) {
  xvc_enc_pic_buffer *rec_pic_ptr{nullptr};
  xvc_enc_nal_unit *nal_units;
  int num_nal_units;

  xvc_enc_return_code ret;

  do {
    ret = xvc_api->encoder_flush(encoder, &nal_units, &num_nal_units,
                                 rec_pic_ptr);
    if (ret != XVC_ENC_OK && ret != XVC_ENC_NO_MORE_OUTPUT) {
      throw std::runtime_error(std::string("xvc flush failed: ") +
                               xvc_api->xvc_enc_get_error_text(ret));
    }

    for (int i = 0; i < num_nal_units; i++) {
      callback(nal_units[i]);
    }

  } while (ret == XVC_ENC_OK);
}

int main(int argc, char *argv[]) {
  const char *input_file_mask{};
  const char *output_file_name{};
  const char *s_qp{};

  std::vector<PPM> images{};
  uint64_t width{};
  uint64_t height{};
  uint32_t color_depth{};
  uint64_t image_count{};

  double qp{};

  std::vector<uint8_t> yuv_frame{};

  char opt{};
  while ((opt = getopt(argc, argv, "hi:o:q:")) >= 0) {
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
      if (!output_file_name) {
        output_file_name = optarg;
        continue;
      }
      break;

    case 'q':
      if (!s_qp) {
        s_qp = optarg;
        continue;
      }
      break;

    default:
      break;
    }

    print_usage(argv[0]);
    throw std::invalid_argument("invalid arguments");
  }

  if (!input_file_mask || !output_file_name) {
    print_usage(argv[0]);
    throw std::invalid_argument("input and output are required");
  }

  qp = 30;
  if (s_qp) {
    std::stringstream input(s_qp);
    if (!(input >> qp) || (input >> std::ws, !input.eof())) {
      print_usage(argv[0]);
      throw std::invalid_argument("QP must be a number");
    }
  }

  images = mapPPMs(input_file_mask);
  width = images.front().width();
  height = images.front().height();
  color_depth = images.front().color_depth();
  image_count = images.size();
  const uint64_t view_side = std::sqrt(image_count);
  if (view_side * view_side != image_count || color_depth > 255) {
    throw std::invalid_argument(
        "input must be a square grid of 8-bit PPM images");
  }

  yuv_frame.resize(width * height * 3 * 2);

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
  params->qp = qp;
  params->chroma_format = XVC_ENC_CHROMA_FORMAT_444;
  params->input_bitdepth = 8;
  params->internal_bitdepth = 8;
  params->framerate = 1;
  params->tune_mode = 1;
  params->threads = -1;

  const xvc_enc_return_code ret = xvc_api->parameters_check(params.get());
  if (ret != XVC_ENC_OK) {
    throw std::invalid_argument(xvc_api->xvc_enc_get_error_text(ret));
  }

  const auto delete_encoder = [xvc_api](xvc_encoder *encoder) noexcept {
    xvc_api->encoder_destroy(encoder);
  };
  std::unique_ptr<xvc_encoder, decltype(delete_encoder)> encoder{
      xvc_api->encoder_create(params.get()), delete_encoder};
  if (!encoder) {
    throw std::runtime_error("xvc encoder creation failed");
  }

  ffmpeg::ScaleContext in_convert_ctx{
      sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height,
                     AV_PIX_FMT_YUV444P, 0, nullptr, nullptr, nullptr),
      sws_freeContext};
  if (!in_convert_ctx) {
    throw std::runtime_error("could not get image conversion context");
  }

  std::ofstream output{};
  auto saveNal = [&](xvc_enc_nal_unit &nal) {
    char nal_size[4]{};

    nal_size[0] = (nal.size >> 0) & 0xFF;
    nal_size[1] = (nal.size >> 8) & 0xFF;
    nal_size[2] = (nal.size >> 16) & 0xFF;
    nal_size[3] = (nal.size >> 24) & 0xFF;

    output.write(nal_size, 4);
    output.write(reinterpret_cast<const char *>(nal.bytes), nal.size);
    if (!output) {
      throw std::ios_base::failure("could not write " +
                                   std::string(output_file_name));
    }
  };

  const std::filesystem::path output_path = output_file_name;
  if (!output_path.parent_path().empty()) {
    std::filesystem::create_directories(output_path.parent_path());
  }

  output.open(output_file_name, std::ios::binary);
  if (!output) {
    throw std::ios_base::failure(
        "could not open " + std::string(output_file_name) + " for writing");
  }

  for (size_t image = 0; image < image_count; image++) {
    const uint8_t *inData[1] = {images[image].pixels().data()};
    uint8_t *outData[3] = {&yuv_frame[width * height * 0],
                           &yuv_frame[width * height * 1],
                           &yuv_frame[width * height * 2]};
    int inLineSize[1] = {static_cast<int>(3 * width)};
    int outLineSize[3] = {static_cast<int>(width), static_cast<int>(width),
                          static_cast<int>(width)};

    if (sws_scale(in_convert_ctx.get(), inData, inLineSize, 0, height, outData,
                  outLineSize) < 0) {
      throw std::runtime_error("RGB to YUV conversion failed");
    }

    encode(xvc_api, encoder.get(), yuv_frame.data(), saveNal);
  }

  flush(xvc_api, encoder.get(), saveNal);

  output.flush();
  output.close();
  if (!output) {
    throw std::ios_base::failure("could not write " +
                                 std::string(output_file_name));
  }

  return 0;
}
