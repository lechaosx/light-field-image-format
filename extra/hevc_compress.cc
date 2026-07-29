/******************************************************************************\
* SOUBOR: hevc_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "plenoppm.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavutil/opt.h>
  #include <libswscale/swscale.h>
}

#include <getopt.h>
#include <cmath>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>


void print_usage(char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0 << " -i <input-file-mask> -o <output-file-name> -b <bitrate>" << std::endl;
}

template <typename F>
void encode(AVCodecContext *context, AVFrame *frame, AVPacket *pkt, F &&callback) {
  if (avcodec_send_frame(context, frame) < 0) {
    std::cerr << "Error sending a frame for encoding" << std::endl;
    exit(1);
  }

  int ret = 0;
  while (ret >= 0) {
    ret = avcodec_receive_packet(context, pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return;
    }
    else if (ret < 0) {
      std::cerr << "Error during encoding" << std::endl;
      exit(1);
    }

    callback(pkt);

    av_packet_unref(pkt);
  }
}

int main(int argc, char *argv[]) {
  const char *input_file_mask  {};
  const char *output_file_name {};
  const char *s_bitrate        {};

  std::vector<PPM> images           {};

  uint64_t width               {};
  uint64_t height              {};
  uint32_t color_depth         {};

  double bitrate               {};
  bool intra_only              {};

  AVPacket *pkt                {};
  AVFrame  *in_frame           {};
  AVFrame  *rgb_frame          {};

  const AVCodec *coder         {};
  AVCodecContext *in_context   {};
  SwsContext *in_convert_ctx   {};

  std::ofstream output              {};

  char opt {};
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
        return 1;
      break;
    }
  }

  if (!input_file_mask || !output_file_name) {
    print_usage(argv[0]);
    return 1;
  }

  bitrate = 0.1;
  try {
    if (s_bitrate) {
      size_t parsed {};
      bitrate = std::stod(s_bitrate, &parsed);
      if (s_bitrate[parsed] != '\0') {
        throw std::invalid_argument("trailing bitrate characters");
      }
    }
  } catch (const std::exception &) {
    std::cerr << "Bitrate must be a number" << std::endl;
    return 1;
  }
  if (!std::isfinite(bitrate) || bitrate <= 0
      || bitrate > static_cast<double>(std::numeric_limits<int64_t>::max())) {
    std::cerr << "Bitrate must be positive and finite" << std::endl;
    return 1;
  }

  try {
    images = mapPPMs(input_file_mask);
  } catch (const std::exception &error) {
    std::cerr << error.what() << std::endl;
    return 2;
  }
  width = images.front().width();
  height = images.front().height();
  color_depth = images.front().color_depth();

  if (color_depth != 255) {
    std::cerr << "Unsupported color depth!" << std::endl;
    return 2;
  }

  pkt = av_packet_alloc();
  if (!pkt) {
    exit(1);
  }

  in_frame = av_frame_alloc();
  if (!in_frame) {
    std::cerr << "Could not allocate video frame" << std::endl;
    exit(1);
  }

  in_frame->format = AV_PIX_FMT_YUV444P;
  in_frame->width  = width;
  in_frame->height = height;

  if (av_frame_get_buffer(in_frame, 32) < 0) {
    std::cerr << "Could not allocate the video frame data" << std::endl;
    exit(1);
  }

  rgb_frame = av_frame_alloc();
  if (!rgb_frame) {
    std::cerr << "Could not allocate video frame" << std::endl;
    exit(1);
  }

  rgb_frame->format = AV_PIX_FMT_RGB24;
  rgb_frame->width  = width;
  rgb_frame->height = height;

  if (av_frame_get_buffer(rgb_frame, 32) < 0) {
    std::cerr << "Could not allocate the video frame data" << std::endl;
    exit(1);
  }

  coder = avcodec_find_encoder(AV_CODEC_ID_H265);
  if (!coder) {
    std::cerr << "coder AV_CODEC_ID_H265 not found" << std::endl;
    exit(1);
  }

  in_context = avcodec_alloc_context3(coder);
  if (!in_context) {
    std::cerr << "Could not allocate video coder context" << std::endl;
    exit(1);
  }

  in_context->width = width;
  in_context->height = height;

  in_context->time_base = {1, 1};
  in_context->framerate = {1, 1};

  in_context->bit_rate = bitrate;

  in_context->pix_fmt = AV_PIX_FMT_YUV444P;

 	av_opt_set(in_context->priv_data, "tune", "psnr", 0);
  av_opt_set(in_context->priv_data, "preset", "placebo", 0);

  in_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height, AV_PIX_FMT_YUV444P, 0, 0, 0, 0);
  if (!in_convert_ctx) {
    std::cerr << "Could not get image conversion context" << std::endl;
    exit(1);
  }

  if (avcodec_open2(in_context, coder, nullptr) < 0) {
    std::cerr << "Could not open coder" << std::endl;
    exit(1);
  }

  const std::filesystem::path output_path = output_file_name;
  if (!output_path.parent_path().empty()) {
    std::filesystem::create_directories(output_path.parent_path());
  }
  
  output.open(output_file_name, std::ios::binary);
  if (!output) {
    std::cerr << "Could not open " << output_file_name << " for writing\n";
    exit(1);
  }

  auto savePkt = [&](AVPacket *pkt) {
    if (!output) {
      return;
    }
    output.write(reinterpret_cast<const char *>(pkt->data), pkt->size);
  };

  bool encoded = true;
  for (size_t image = 0; image < images.size(); ++image) {
    const uint8_t *inData[1] = {images[image].pixels().data()};
    int      inLinesize[1] = { static_cast<int>(3 * width) };

    sws_scale(in_convert_ctx, inData, inLinesize, 0, height, in_frame->data, in_frame->linesize);

    if (intra_only) {
      in_frame->pict_type = AV_PICTURE_TYPE_I;
    }

    in_frame->pts = image;

    encode(in_context, in_frame, pkt, savePkt);
    if (!output) {
      encoded = false;
      break;
    }
  }

  if (encoded) {
    encode(in_context, nullptr, pkt, savePkt);
  }

  output.flush();
  output.close();
  if (!output) {
    std::cerr << "Could not write " << output_file_name << std::endl;
    encoded = false;
  }

  sws_freeContext(in_convert_ctx);
  avcodec_free_context(&in_context);
  av_frame_free(&in_frame);
  av_frame_free(&rgb_frame);
  av_packet_free(&pkt);

  return encoded ? 0 : 1;
}
