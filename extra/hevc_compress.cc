/******************************************************************************\
* SOUBOR: hevc_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "ffmpeg_raii.h"
#include "plenoppm.h"

extern "C" {
#include <libavutil/opt.h>
}

#include <cmath>
#include <getopt.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

void print_usage(char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0
            << " -i <input-file-mask> -o <output-file-name> -b <bitrate>"
            << std::endl;
}

template <typename F>
void encode(AVCodecContext *context, AVFrame *frame, AVPacket *pkt,
            F &&callback) {
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

int main(int argc, char *argv[]) {
  const char *input_file_mask{};
  const char *output_file_name{};
  const char *s_bitrate{};

  std::vector<PPM> images{};

  int width{};
  int height{};
  uint32_t color_depth{};

  double bitrate{};
  bool intra_only{};

  char opt{};
  while ((opt = getopt(argc, argv, "hi:o:b:I")) >= 0) {
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

    case 'b':
      if (!s_bitrate) {
        s_bitrate = optarg;
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

  if (!input_file_mask || !output_file_name) {
    print_usage(argv[0]);
    throw std::invalid_argument("input and output are required");
  }

  bitrate = 0.1;
  try {
    if (s_bitrate) {
      size_t parsed{};
      bitrate = std::stod(s_bitrate, &parsed);
      if (s_bitrate[parsed] != '\0') {
        throw std::invalid_argument("trailing bitrate characters");
      }
    }
  } catch (const std::exception &) {
    throw std::invalid_argument("bitrate must be a number");
  }
  if (!std::isfinite(bitrate) || bitrate <= 0 ||
      bitrate > static_cast<double>(std::numeric_limits<int64_t>::max())) {
    throw std::invalid_argument("bitrate must be positive and finite");
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

  if (color_depth != 255) {
    throw std::invalid_argument("unsupported color depth");
  }

  ffmpeg::Packet pkt{av_packet_alloc()};
  ffmpeg::Frame in_frame{av_frame_alloc()};
  if (!pkt || !in_frame) {
    throw std::runtime_error("could not allocate FFmpeg frame or packet");
  }

  in_frame->format = AV_PIX_FMT_YUV444P;
  in_frame->width = width;
  in_frame->height = height;
  const int buffer_status = av_frame_get_buffer(in_frame.get(), 32);
  if (buffer_status < 0) {
    throw std::runtime_error("could not allocate input frame data: " +
                             std::to_string(buffer_status));
  }

  const AVCodec *coder = avcodec_find_encoder(AV_CODEC_ID_H265);
  if (!coder) {
    throw std::runtime_error("H.265 encoder not found");
  }

  ffmpeg::CodecContext in_context{avcodec_alloc_context3(coder)};
  if (!in_context) {
    throw std::runtime_error("could not allocate video encoder context");
  }

  in_context->width = width;
  in_context->height = height;
  in_context->time_base = {1, 1};
  in_context->framerate = {1, 1};
  in_context->bit_rate = bitrate;
  in_context->pix_fmt = AV_PIX_FMT_YUV444P;

  const int tune_status = av_opt_set(in_context->priv_data, "tune", "psnr", 0);
  const int preset_status =
      av_opt_set(in_context->priv_data, "preset", "placebo", 0);
  if (tune_status < 0 || preset_status < 0) {
    throw std::runtime_error("could not configure H.265 encoder");
  }

  ffmpeg::ScaleContext in_convert_ctx{
      sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height,
                     AV_PIX_FMT_YUV444P, 0, nullptr, nullptr, nullptr),
      sws_freeContext};
  if (!in_convert_ctx) {
    throw std::runtime_error("could not create image conversion context");
  }

  const int encoder_status = avcodec_open2(in_context.get(), coder, nullptr);
  if (encoder_status < 0) {
    throw std::runtime_error("could not open encoder: " +
                             std::to_string(encoder_status));
  }

  const std::filesystem::path output_path = output_file_name;
  if (!output_path.parent_path().empty()) {
    std::filesystem::create_directories(output_path.parent_path());
  }

  std::ofstream output{output_file_name, std::ios::binary};
  if (!output) {
    throw std::runtime_error("could not open " + std::string(output_file_name) +
                             " for writing");
  }

  auto savePkt = [&](AVPacket *packet) {
    output.write(reinterpret_cast<const char *>(packet->data), packet->size);
    if (!output) {
      throw std::runtime_error("could not write " +
                               std::string(output_file_name));
    }
  };

  for (size_t image = 0; image < images.size(); ++image) {
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
    encode(in_context.get(), in_frame.get(), pkt.get(), savePkt);
  }

  encode(in_context.get(), nullptr, pkt.get(), savePkt);

  output.flush();
  output.close();
  if (!output) {
    throw std::runtime_error("could not write " +
                             std::string(output_file_name));
  }

  return 0;
}
