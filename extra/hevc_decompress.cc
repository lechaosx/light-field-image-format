/******************************************************************************\
* SOUBOR: hevc_decompress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "ffmpeg_raii.h"
#include "file_mask.h"
#include "plenoppm.h"

#include <cmath>
#include <getopt.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

void print_usage(char *argv0) {
  std::cerr << "Usage: " << std::endl;
  std::cerr << argv0 << " -i <input-file-name> -o <output-file-mask>"
            << std::endl;
}

template <typename F>
void decode(AVCodecContext *context, AVFrame *frame, AVPacket *pkt,
            F &&callback) {
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
  const char *input_file_name{};
  const char *output_file_mask{};

  std::ifstream input{};

  constexpr size_t input_buffer_size = 4096;
  std::array<uint8_t, input_buffer_size + AV_INPUT_BUFFER_PADDING_SIZE>
      input_buffer{};

  char opt{};
  while ((opt = getopt(argc, argv, "hi:o:")) >= 0) {
    switch (opt) {
    case 'h':
      print_usage(argv[0]);
      return 0;

    case 'i':
      if (!input_file_name) {
        input_file_name = optarg;
        continue;
      }
      break;

    case 'o':
      if (!output_file_mask) {
        output_file_mask = optarg;
        continue;
      }
      break;

    default:
      print_usage(argv[0]);
      throw std::invalid_argument("invalid arguments");
      break;
    }
  }

  if (!input_file_name || !output_file_mask) {
    print_usage(argv[0]);
    throw std::invalid_argument("input and output are required");
  }

  size_t view_counter{0};
  const FileMask output_names{output_file_mask};
  const size_t output_name_count = output_names.count();
  if (output_name_count == 0) {
    throw std::invalid_argument("output file mask is too large");
  }

  ffmpeg::Packet pkt{av_packet_alloc()};
  ffmpeg::Frame out_frame{av_frame_alloc()};
  if (!pkt || !out_frame) {
    throw std::runtime_error("could not allocate FFmpeg frame or packet");
  }

  const AVCodec *decoder = avcodec_find_decoder(AV_CODEC_ID_H265);
  if (!decoder) {
    throw std::runtime_error("H.265 decoder not found");
  }

  ffmpeg::Parser parser{av_parser_init(decoder->id), av_parser_close};
  if (!parser) {
    throw std::runtime_error("H.265 parser not found");
  }

  ffmpeg::CodecContext out_context{avcodec_alloc_context3(decoder)};
  if (!out_context) {
    throw std::runtime_error("could not allocate video decoder context");
  }

  const int decoder_status = avcodec_open2(out_context.get(), decoder, nullptr);
  if (decoder_status < 0) {
    throw std::runtime_error("could not open decoder: " +
                             std::to_string(decoder_status));
  }

  auto saveFrame = [&](AVFrame *frame) {
    if (frame->width <= 0 || frame->height <= 0
        || frame->width > std::numeric_limits<int>::max() / 3) {
      throw std::runtime_error("decoded frame dimensions exceed codec limits");
    }
    if (view_counter >= output_name_count) {
      throw std::runtime_error("file mask cannot represent frame " +
                               std::to_string(view_counter));
    }

    ffmpeg::Frame rgb_frame{av_frame_alloc()};
    if (!rgb_frame) {
      throw std::runtime_error("could not allocate RGB frame");
    }

    rgb_frame->format = AV_PIX_FMT_RGB24;
    rgb_frame->width = frame->width;
    rgb_frame->height = frame->height;

    const int buffer_status = av_frame_get_buffer(rgb_frame.get(), 32);
    if (buffer_status < 0) {
      throw std::runtime_error("could not allocate RGB frame data: " +
                               std::to_string(buffer_status));
    }

    ffmpeg::ScaleContext convert_context{
        sws_getContext(frame->width, frame->height,
                       static_cast<AVPixelFormat>(frame->format),
                       rgb_frame->width, rgb_frame->height, AV_PIX_FMT_RGB24, 0,
                       nullptr, nullptr, nullptr),
        sws_freeContext};
    if (!convert_context) {
      throw std::runtime_error("could not create image conversion context");
    }

    int outLinesize[1] = {3 * frame->width};
    const int scale_status =
        sws_scale(convert_context.get(), frame->data, frame->linesize, 0,
                  frame->height, rgb_frame->data, outLinesize);
    if (scale_status < 0) {
      throw std::runtime_error("could not convert decoded frame: " +
                               std::to_string(scale_status));
    }

    const std::string filename = output_names[view_counter++];
    const std::filesystem::path output_path = filename;
    if (!output_path.parent_path().empty()) {
      std::filesystem::create_directories(output_path.parent_path());
    }

    PPM ppm =
        PPM::create(output_path, rgb_frame->width, rgb_frame->height, 255);
    for (size_t pixel = 0; pixel < ppm.width() * ppm.height(); ++pixel) {
      ppm.put(pixel, {rgb_frame->data[0][pixel * 3 + 0],
                      rgb_frame->data[0][pixel * 3 + 1],
                      rgb_frame->data[0][pixel * 3 + 2]});
    }
    ppm.flush();
  };

  input.open(input_file_name, std::ios::binary);
  if (!input) {
    throw std::runtime_error("could not open " + std::string(input_file_name) +
                             " for reading");
  }

  while (input) {
    input.read(reinterpret_cast<char *>(input_buffer.data()),
               input_buffer_size);
    size_t data_size = input.gcount();
    if (!data_size) {
      break;
    }

    uint8_t *data = input_buffer.data();
    while (data_size > 0) {
      const int parsed = av_parser_parse2(
          parser.get(), out_context.get(), &pkt->data, &pkt->size, data,
          data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
      if (parsed < 0) {
        throw std::runtime_error("error while parsing HEVC input: " +
                                 std::to_string(parsed));
      }
      if (parsed == 0 && !pkt->size) {
        throw std::runtime_error("HEVC parser made no progress");
      }

      data += parsed;
      data_size -= parsed;

      if (pkt->size) {
        decode(out_context.get(), out_frame.get(), pkt.get(), saveFrame);
      }
    }
  }
  if (input.bad()) {
    throw std::runtime_error("could not read " + std::string(input_file_name));
  }

  const int parser_status =
      av_parser_parse2(parser.get(), out_context.get(), &pkt->data, &pkt->size,
                       nullptr, 0, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
  if (parser_status < 0) {
    throw std::runtime_error("error while draining HEVC parser: " +
                             std::to_string(parser_status));
  }
  if (pkt->size) {
    decode(out_context.get(), out_frame.get(), pkt.get(), saveFrame);
  }
  decode(out_context.get(), out_frame.get(), nullptr, saveFrame);
  if (view_counter == 0) {
    throw std::runtime_error("no HEVC frames decoded");
  }

  return 0;
}
