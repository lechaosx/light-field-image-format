#include "ffmpeg_raii.h"
#include "plenoppm.h"

extern "C" {
#include <libavutil/opt.h>
}

#include <cmath>
#include <cstdio>
#include <getopt.h>

#include <fstream>
#include <functional>
#include <limits>
#include <print>
#include <stdexcept>
#include <string>

void print_usage(char *argv0) {
  std::println(stderr, "Usage:");
  std::println(
      stderr,
      "{} -i <input-file-mask> -o <output-file-name> [-f "
      "<fist-bitrate>] [-l <last-bitrate>] [-a] [-I]",
      argv0);
}

void encode(AVCodecContext *context, AVFrame *frame, AVPacket *pkt,
            const std::function<void(AVPacket *)> &callback) {
  const int send_status = avcodec_send_frame(context, frame);
  if (send_status < 0) {
    throw std::runtime_error("error sending a frame for encoding: " +
                             std::to_string(send_status));
  }

  while (true) {
    const int receive_status = avcodec_receive_packet(context, pkt);
    if (receive_status == AVERROR(EAGAIN) || receive_status == AVERROR_EOF) {
      return;
    }
    if (receive_status < 0) {
      throw std::runtime_error("error during encoding: " +
                               std::to_string(receive_status));
    }

    callback(pkt);

    av_packet_unref(pkt);
  }
}

void decode(AVCodecContext *context, AVFrame *frame, AVPacket *pkt,
            const std::function<void(AVFrame *)> &callback) {
  const int send_status = avcodec_send_packet(context, pkt);
  if (send_status < 0) {
    throw std::runtime_error("error sending a packet for decoding: " +
                             std::to_string(send_status));
  }

  while (true) {
    const int receive_status = avcodec_receive_frame(context, frame);
    if (receive_status == AVERROR(EAGAIN) || receive_status == AVERROR_EOF) {
      return;
    }
    if (receive_status < 0) {
      throw std::runtime_error("error during decoding: " +
                               std::to_string(receive_status));
    }

    callback(frame);
  }
}

int main(int argc, char *argv[]) {
  const char *input_file_mask{};
  const char *output_file{};
  const char *first_bitrate{};
  const char *last_bitrate{};

  std::vector<PPM> images{};

  int width{};
  int height{};
  uint32_t color_depth{};
  size_t image_count{};

  size_t image_pixels{};

  double f_b{};
  double l_b{};

  bool append{};
  bool intra_only{};

  char opt{};
  while ((opt = getopt(argc, argv, "hi:o:f:l:aI")) >= 0) {
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

    case 'I':
      if (!intra_only) {
        intra_only = true;
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

  f_b = 0.1;
  l_b = 15;
  try {
    if (first_bitrate) {
      size_t parsed{};
      f_b = std::stod(first_bitrate, &parsed);
      if (first_bitrate[parsed] != '\0') {
        throw std::invalid_argument("trailing bitrate characters");
      }
    }
    if (last_bitrate) {
      size_t parsed{};
      l_b = std::stod(last_bitrate, &parsed);
      if (last_bitrate[parsed] != '\0') {
        throw std::invalid_argument("trailing bitrate characters");
      }
    }
  } catch (const std::exception &) {
    throw std::invalid_argument("bitrates must be numbers");
  }
  if (!std::isfinite(f_b) || !std::isfinite(l_b) || f_b <= 0 || f_b > l_b) {
    throw std::invalid_argument(
        "bitrates must be positive and ordered from first to last");
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

  ffmpeg::Packet pkt{av_packet_alloc()};
  ffmpeg::Frame out_frame{av_frame_alloc()};
  ffmpeg::Frame in_frame{av_frame_alloc()};
  ffmpeg::Frame rgb_frame{av_frame_alloc()};
  if (!pkt || !out_frame || !in_frame || !rgb_frame) {
    throw std::runtime_error("could not allocate FFmpeg frame or packet");
  }

  in_frame->format = AV_PIX_FMT_YUV444P;
  in_frame->width = width;
  in_frame->height = height;
  const int input_buffer_status = av_frame_get_buffer(in_frame.get(), 32);
  if (input_buffer_status < 0) {
    throw std::runtime_error("could not allocate input frame data: " +
                             std::to_string(input_buffer_status));
  }

  rgb_frame->format = AV_PIX_FMT_RGB24;
  rgb_frame->width = width;
  rgb_frame->height = height;
  const int rgb_buffer_status = av_frame_get_buffer(rgb_frame.get(), 32);
  if (rgb_buffer_status < 0) {
    throw std::runtime_error("could not allocate RGB frame data: " +
                             std::to_string(rgb_buffer_status));
  }

  const AVCodec *decoder = avcodec_find_decoder(AV_CODEC_ID_H265);
  const AVCodec *coder = avcodec_find_encoder(AV_CODEC_ID_H265);
  if (!decoder || !coder) {
    throw std::runtime_error("H.265 encoder or decoder not found");
  }

  ffmpeg::ScaleContext in_convert_ctx{
      sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height,
                     AV_PIX_FMT_YUV444P, 0, nullptr, nullptr, nullptr),
      sws_freeContext};
  ffmpeg::ScaleContext out_convert_ctx{
      sws_getContext(width, height, AV_PIX_FMT_YUV444P, width, height,
                     AV_PIX_FMT_RGB24, 0, nullptr, nullptr, nullptr),
      sws_freeContext};
  if (!in_convert_ctx || !out_convert_ctx) {
    throw std::runtime_error("could not create image conversion context");
  }

  std::ofstream output{};
  if (append) {
    output.open(output_file, std::fstream::app);
  } else {
    output.open(output_file, std::fstream::trunc);
    output << "'x265' 'PSNR [dB]' 'bitrate [bpp]'" << std::endl;
  }
  if (!output) {
    throw std::runtime_error("could not open " + std::string(output_file) +
                             " for writing");
  }

  for (double bpp = f_b; bpp <= l_b; bpp *= 1.25893) {
    size_t compressed_size = 0;

    ffmpeg::CodecContext in_context{avcodec_alloc_context3(coder)};
    ffmpeg::CodecContext out_context{avcodec_alloc_context3(decoder)};
    if (!in_context || !out_context) {
      throw std::runtime_error("could not allocate video codec context");
    }

    in_context->width = width;
    in_context->height = height;
    if (image_count > static_cast<size_t>(std::numeric_limits<int>::max())) {
      throw std::length_error("PPM grid contains too many images");
    }
    in_context->time_base = {1, static_cast<int>(image_count)};
    in_context->framerate = {static_cast<int>(image_count), 1};
    in_context->pix_fmt = AV_PIX_FMT_YUV444P;
    const double bitrate = bpp * image_pixels;
    if (bitrate >= static_cast<double>(std::numeric_limits<int64_t>::max())) {
      throw std::length_error("requested bitrate exceeds codec limits");
    }
    in_context->bit_rate = static_cast<int64_t>(bitrate);

    const int tune_status =
        av_opt_set(in_context->priv_data, "tune", "psnr", 0);
    const int preset_status =
        av_opt_set(in_context->priv_data, "preset", "placebo", 0);
    if (tune_status < 0 || preset_status < 0) {
      throw std::runtime_error("could not configure x265 encoder");
    }

    const int encoder_status = avcodec_open2(in_context.get(), coder, nullptr);
    if (encoder_status < 0) {
      throw std::runtime_error("could not open encoder: " +
                               std::to_string(encoder_status));
    }

    const int decoder_status =
        avcodec_open2(out_context.get(), decoder, nullptr);
    if (decoder_status < 0) {
      throw std::runtime_error("could not open decoder: " +
                               std::to_string(decoder_status));
    }

    double mse = 0;
    size_t input_image = 0;

    auto saveFrame = [&](AVFrame *frame) {
      if (input_image >= images.size()) {
        throw std::runtime_error(
            "decoder produced more frames than the input contains");
      }
      int outLinesize[1] = {3 * width};
      const int scale_status =
          sws_scale(out_convert_ctx.get(), frame->data, frame->linesize, 0,
                    height, rgb_frame->data, outLinesize);
      if (scale_status < 0) {
        throw std::runtime_error("could not convert decoded frame: " +
                                 std::to_string(scale_status));
      }

      const auto *original = images[input_image].pixels().data();
      for (size_t pix = 0; pix < frame_pixels * 3; ++pix) {
        double tmp = original[pix] - rgb_frame->data[0][pix];
        mse += tmp * tmp;
      }
      ++input_image;
    };

    auto decodePkt = [&](AVPacket *pkt) {
      if (pkt) {
        compressed_size += pkt->size;
      }
      decode(out_context.get(), out_frame.get(), pkt, saveFrame);
    };

    for (size_t image = 0; image < image_count; image++) {
      const uint8_t *inData[1] = {images[image].pixels().data()};
      int inLinesize[1] = {3 * width};
      const int scale_status =
          sws_scale(in_convert_ctx.get(), inData, inLinesize, 0, height,
                    in_frame->data, in_frame->linesize);
      if (scale_status < 0) {
        throw std::runtime_error("could not convert input frame: " +
                                 std::to_string(scale_status));
      }

      if (intra_only) {
        in_frame->pict_type = AV_PICTURE_TYPE_I;
      }

      in_frame->pts = image;

      encode(in_context.get(), in_frame.get(), pkt.get(), decodePkt);
    }

    encode(in_context.get(), nullptr, pkt.get(), decodePkt);
    decodePkt(nullptr);

    mse /= image_pixels * 3;

    double out_bpp = compressed_size * 8.0 / image_pixels;
    double psnr = 10 * log10((255 * 255) / mse);

    std::println(stderr, "{} {} {}", bpp, psnr, out_bpp);
    output << bpp << " " << psnr << " " << out_bpp << std::endl;

    if (std::abs(bpp - out_bpp) > 2) {
      break;
    }
  }

  output.flush();
  output.close();
  if (!output) {
    throw std::runtime_error("could not write " + std::string(output_file));
  }

  return 0;
}
