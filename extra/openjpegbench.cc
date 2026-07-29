/******************************************************************************\
* SOUBOR: openjpegbench.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

#include <getopt.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <openjpeg.h>
#include <unistd.h>

namespace {

struct TemporaryFile {
  std::filesystem::path path;

  TemporaryFile() {
    const std::filesystem::path pattern =
        std::filesystem::temp_directory_path() / "lfif-openjpeg-XXXXXX";
    const std::string pattern_string = pattern.string();
    std::vector<char> writable_pattern(pattern_string.begin(),
                                       pattern_string.end());
    writable_pattern.push_back('\0');
    const int descriptor = mkstemp(writable_pattern.data());
    if (descriptor < 0) {
      throw std::runtime_error("Could not create temporary OpenJPEG file");
    }
    const auto rollback = [&writable_pattern](int *file) noexcept {
      if (*file >= 0) {
        close(*file);
      }
      unlink(writable_pattern.data());
    };
    int owned_descriptor = descriptor;
    std::unique_ptr<int, decltype(rollback)> temporary_file {
      &owned_descriptor, rollback
    };
    path = writable_pattern.data();
    const int close_status = close(owned_descriptor);
    const int close_error = errno;
    owned_descriptor = -1;
    if (close_status < 0) {
      throw std::system_error(
          close_error, std::generic_category(), "could not close temporary OpenJPEG file");
    }
    temporary_file.release();
  }

  ~TemporaryFile() {
    std::error_code error;
    std::filesystem::remove(path, error);
  }
};

}

void print_usage(char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0
            << " -i <input-file-mask> -o <output-file-name> [-f <fist-psnr>] "
               "[-l <last-psnr>] [-s <psnr-step>] [-a]"
            << std::endl;
}

static void error_callback(const char *msg, void *) {
  std::cerr << "[ERROR] " << msg;
}

static void warning_callback(const char *msg, void *) {
  std::cerr << "[WARNING] " << msg;
}

static void info_callback(const char *, void *) {}

int main(int argc, char *argv[]) {
  const char *input_file_mask{};
  const char *output_file{};
  const char *param_psnr_first{};
  const char *param_psnr_last{};

  const char *param_psnr_step{};

  float psnr_step{};
  float psnr_first{};
  float psnr_last{};
  bool append{};

  std::vector<PPM> images{};

  OPJ_UINT32 width{};
  OPJ_UINT32 height{};
  uint32_t color_depth{};
  size_t image_count{};

  opj_cparameters_t cparameters{};
  opj_dparameters_t dparameters{};
  opj_image_cmptparm_t cmptparm[3]{};
  char opt{};
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
      throw std::invalid_argument("invalid arguments");
      break;
    }
  }

  if (!input_file_mask || !output_file) {
    print_usage(argv[0]);
    throw std::invalid_argument("input and output are required");
  }

  psnr_step = 1.0;
  psnr_first = 20.f;
  psnr_last = 50.f;
  try {
    if (param_psnr_step) {
      size_t parsed{};
      psnr_step = std::stof(param_psnr_step, &parsed);
      if (param_psnr_step[parsed] != '\0') {
        throw std::invalid_argument("trailing PSNR characters");
      }
    }
    if (param_psnr_first) {
      size_t parsed{};
      psnr_first = std::stof(param_psnr_first, &parsed);
      if (param_psnr_first[parsed] != '\0') {
        throw std::invalid_argument("trailing PSNR characters");
      }
    }
    if (param_psnr_last) {
      size_t parsed{};
      psnr_last = std::stof(param_psnr_last, &parsed);
      if (param_psnr_last[parsed] != '\0') {
        throw std::invalid_argument("trailing PSNR characters");
      }
    }
  } catch (const std::exception &) {
    throw std::invalid_argument("PSNR values must be numbers");
  }
  if (!std::isfinite(psnr_step) || !std::isfinite(psnr_first) ||
      !std::isfinite(psnr_last) || psnr_step <= 0 || psnr_first <= 0 ||
      psnr_first > psnr_last || psnr_first + psnr_step == psnr_first) {
    throw std::invalid_argument("PSNR values must be positive, increasing and "
                                "ordered from first to last");
  }

  images = mapPPMs(input_file_mask);
  const uint64_t input_width = images.front().width();
  const uint64_t input_height = images.front().height();
  if (input_width > std::numeric_limits<OPJ_UINT32>::max()
      || input_height > std::numeric_limits<OPJ_UINT32>::max()) {
    throw std::invalid_argument("PPM dimensions exceed codec limits");
  }
  width = static_cast<OPJ_UINT32>(input_width);
  height = static_cast<OPJ_UINT32>(input_height);
  color_depth = images.front().color_depth();
  image_count = images.size();
  const size_t view_side = std::sqrt(image_count);
  if (view_side * view_side != image_count || color_depth > 255) {
    throw std::invalid_argument(
        "input must be a square grid of 8-bit PPM images");
  }

  opj_set_default_encoder_parameters(&cparameters);
  cparameters.cod_format = OPJ_CODEC_J2K;
  cparameters.tcp_numlayers = 1;
  cparameters.cp_fixed_quality = 1;
  cparameters.irreversible = 1;
  cparameters.numresolution = 1;
  for (uint64_t extent = std::min(width, height); extent > 1; extent >>= 1) {
    cparameters.numresolution++;
  }

  opj_set_default_decoder_parameters(&dparameters);
  dparameters.decod_format = OPJ_CODEC_J2K;
  dparameters.cp_layer = 0;
  dparameters.cp_reduce = 0;

  for (size_t i = 0; i < 3; i++) {
    cmptparm[i].dx = 1;
    cmptparm[i].dy = 1;
    cmptparm[i].w = width;
    cmptparm[i].h = height;
    cmptparm[i].x0 = 0;
    cmptparm[i].y0 = 0;
    cmptparm[i].prec = 8;
    cmptparm[i].sgnd = 0;
  }

  std::ofstream output{};
  if (append) {
    output.open(output_file, std::fstream::app);
  } else {
    output.open(output_file, std::fstream::trunc);
    output << "'openjpeg' 'PSNR [dB]' 'bitrate [bpp]'" << std::endl;
  }
  if (!output) {
    throw std::ios_base::failure("could not open " + std::string(output_file) +
                                 " for writing");
  }

  const size_t frame_pixels = static_cast<size_t>(width) * height;
  if (image_count > std::numeric_limits<size_t>::max() / 3
      || frame_pixels > std::numeric_limits<size_t>::max() / (image_count * 3)) {
    throw std::length_error("PPM grid is too large");
  }
  const size_t image_pixels = frame_pixels * image_count;
  TemporaryFile temporary_file;

  for (float param_psnr = psnr_first; param_psnr <= psnr_last;
       param_psnr += psnr_step) {
    std::cerr << "PSNR: " << param_psnr << "\n";

    double mse = 0;
    size_t compressed_size = 0;

    cparameters.tcp_distoratio[0] = param_psnr;

    for (size_t img = 0; img < image_count; img++) {
      {
        std::unique_ptr<opj_image_t, decltype(&opj_image_destroy)> image{
            opj_image_create(3, cmptparm, OPJ_CLRSPC_SRGB), opj_image_destroy};
        if (!image) {
          throw std::runtime_error("could not create OpenJPEG image");
        }
        image->x0 = 0;
        image->y0 = 0;
        image->x1 = width;
        image->y1 = height;
        image->color_space = OPJ_CLRSPC_SRGB;

        for (size_t pixel = 0; pixel < frame_pixels; pixel++) {
          const auto rgb = images[img].get(pixel);
          for (size_t component = 0; component < 3; component++) {
            image->comps[component].data[pixel] = rgb[component];
          }
        }

        std::unique_ptr<opj_stream_t, decltype(&opj_stream_destroy)> stream{
            opj_stream_create_default_file_stream(temporary_file.path.c_str(),
                                                  OPJ_FALSE),
            opj_stream_destroy};
        std::unique_ptr<opj_codec_t, decltype(&opj_destroy_codec)> codec{
            opj_create_compress(OPJ_CODEC_J2K), opj_destroy_codec};
        if (!stream || !codec) {
          throw std::runtime_error("could not create OpenJPEG encoder");
        }
        if (!opj_set_info_handler(codec.get(), info_callback, nullptr) ||
            !opj_set_warning_handler(codec.get(), warning_callback, nullptr) ||
            !opj_set_error_handler(codec.get(), error_callback, nullptr)) {
          throw std::runtime_error(
              "could not configure OpenJPEG encoder logging");
        }

        const OPJ_BOOL encoded =
            opj_setup_encoder(codec.get(), &cparameters, image.get()) &&
            opj_start_compress(codec.get(), image.get(), stream.get()) &&
            opj_encode(codec.get(), stream.get()) &&
            opj_end_compress(codec.get(), stream.get());
        if (!encoded) {
          throw std::runtime_error("OpenJPEG encoding failed");
        }
      }

      std::ifstream in(temporary_file.path,
                       std::ifstream::ate | std::ifstream::binary);
      if (!in || in.tellg() < 0) {
        throw std::runtime_error("could not read OpenJPEG output");
      }
      compressed_size += static_cast<size_t>(in.tellg());

      std::unique_ptr<opj_stream_t, decltype(&opj_stream_destroy)> input_stream{
          opj_stream_create_default_file_stream(temporary_file.path.c_str(),
                                                OPJ_TRUE),
          opj_stream_destroy};
      std::unique_ptr<opj_codec_t, decltype(&opj_destroy_codec)> decoder{
          opj_create_decompress(OPJ_CODEC_J2K), opj_destroy_codec};
      if (!input_stream || !decoder) {
        throw std::runtime_error("could not create OpenJPEG decoder");
      }
      if (!opj_set_info_handler(decoder.get(), info_callback, nullptr) ||
          !opj_set_warning_handler(decoder.get(), warning_callback, nullptr) ||
          !opj_set_error_handler(decoder.get(), error_callback, nullptr) ||
          !opj_codec_set_threads(decoder.get(), 4)) {
        throw std::runtime_error("could not configure OpenJPEG decoder");
      }

      opj_image_t *decoded_image_pointer{};
      const OPJ_BOOL header_read =
          opj_setup_decoder(decoder.get(), &dparameters) &&
          opj_read_header(input_stream.get(), decoder.get(),
                          &decoded_image_pointer);
      std::unique_ptr<opj_image_t, decltype(&opj_image_destroy)> decoded_image{
          decoded_image_pointer, opj_image_destroy};
      if (!header_read || !decoded_image ||
          !opj_decode(decoder.get(), input_stream.get(), decoded_image.get()) ||
          !opj_end_decompress(decoder.get(), input_stream.get())) {
        throw std::runtime_error("OpenJPEG decoding failed");
      }

      for (size_t pixel = 0; pixel < frame_pixels; pixel++) {
        const auto rgb = images[img].get(pixel);
        for (size_t component = 0; component < 3; component++) {
          double tmp =
              rgb[component] - decoded_image->comps[component].data[pixel];
          mse += tmp * tmp;
        }
      }
    }

    mse /= image_pixels * 3;

    double bpp = compressed_size * 8.0 / image_pixels;
    double psnr = 10 * log10((255 * 255) / mse);

    std::cerr << param_psnr << " " << psnr << " " << bpp << std::endl;
    output << param_psnr << " " << psnr << " " << bpp << std::endl;
  }

  output.flush();
  output.close();
  if (!output) {
    throw std::ios_base::failure("could not write " + std::string(output_file));
  }
  return 0;
}
