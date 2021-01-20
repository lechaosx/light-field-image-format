/******************************************************************************\
* SOUBOR: hevc_compress.cc
* AUTOR: Drahomir Dlabaja (xdlaba02)
\******************************************************************************/

#include "file_mask.h"

#include <ppm.h>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavutil/opt.h>
  #include <libswscale/swscale.h>
}

#include <getopt.h>
#include <cmath>

#include <iostream>
#include <fstream>
#include <functional>
#include <iostream>

using namespace std;

void print_usage(char *argv0) {
  cerr << "Usage: " << endl;
  cerr << argv0 << " -i <input-file-mask> -o <output-file-name> -b <bitrate>" << endl;
}

template <typename F>
void encode(AVCodecContext *context, AVFrame *frame, AVPacket *pkt, F &&callback) {
  if (avcodec_send_frame(context, frame) < 0) {
    cerr << "Error sending a frame for encoding" << endl;
    exit(1);
  }

  int ret = 0;
  while (ret >= 0) {
    ret = avcodec_receive_packet(context, pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return;
    }
    else if (ret < 0) {
      cerr << "Error during encoding" << endl;
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

  PPMFileStruct   ppm          {};
  Pixel *ppm_row               {};
  vector<uint8_t> rgb_data     {};

  uint64_t width               {};
  uint64_t height              {};
  uint32_t color_depth         {};

  double bitrate               {};
  bool intra_only              {};

  AVPacket *pkt                {};
  AVFrame  *in_frame           {};
  AVFrame  *rgb_frame          {};

  AVCodec *coder               {};
  AVCodecContext *in_context   {};
  SwsContext *in_convert_ctx   {};

  ofstream output              {};

  char opt {};
  while ((opt = getopt(argc, argv, "i:o:b:I")) >= 0) {
    switch (opt) {
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
  if (s_bitrate) {
    bitrate = stod(s_bitrate);
  }

  for (size_t i = 0; i < get_mask_names_count(input_file_mask, '#'); i++) {
    ppm.file = fopen(get_name_from_mask(input_file_mask, '#', 0).c_str(), "rb");
    if (ppm.file) {
      break;
    }
  }

  if (!ppm.file) {
    cerr << "ERROR: NO IMAGE LOADED" << endl;
    return 2;
  }

  if (readPPMHeader(&ppm)) {
    cerr << "ERROR: BAD PPM HEADER" << endl;
    return 2;
  }

  fclose(ppm.file);

  width       = ppm.width;
  height      = ppm.height;
  color_depth = ppm.color_depth;

  if (color_depth != 255) {
    cerr << "Unsupported color depth!" << endl;
    return 2;
  }

  rgb_data.resize(width * height * 3);
  ppm_row = allocPPMRow(width);

  pkt = av_packet_alloc();
  if (!pkt) {
    exit(1);
  }

  in_frame = av_frame_alloc();
  if (!in_frame) {
    cerr << "Could not allocate video frame" << endl;
    exit(1);
  }

  in_frame->format = AV_PIX_FMT_YUV444P;
  in_frame->width  = width;
  in_frame->height = height;

  if (av_frame_get_buffer(in_frame, 32) < 0) {
    cerr << "Could not allocate the video frame data" << endl;
    exit(1);
  }

  rgb_frame = av_frame_alloc();
  if (!rgb_frame) {
    cerr << "Could not allocate video frame" << endl;
    exit(1);
  }

  rgb_frame->format = AV_PIX_FMT_RGB24;
  rgb_frame->width  = width;
  rgb_frame->height = height;

  if (av_frame_get_buffer(rgb_frame, 32) < 0) {
    cerr << "Could not allocate the video frame data" << endl;
    exit(1);
  }

  avcodec_register_all();
  coder = avcodec_find_encoder(AV_CODEC_ID_H265);
  if (!coder) {
    cerr << "coder AV_CODEC_ID_H265 not found" << endl;
    exit(1);
  }

  in_context = avcodec_alloc_context3(coder);
  if (!in_context) {
    cerr << "Could not allocate video coder context" << endl;
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
    cerr << "Could not get image conversion context" << endl;
    exit(1);
  }

  if (avcodec_open2(in_context, coder, nullptr) < 0) {
    cerr << "Could not open coder" << endl;
    exit(1);
  }

  if (create_directory(output_file_name)) {
    cerr << "ERROR: CANNON OPEN " << output_file_name << " FOR WRITING\n";
    return 1;
  }
  
  output.open(output_file_name, ios::binary);
  if (!output) {
    cerr << "Could not open " << output_file_name << " for writing\n";
    exit(1);
  }

  auto savePkt = [&](AVPacket *pkt) {
    output.write(reinterpret_cast<const char *>(pkt->data), pkt->size);
  };

  size_t view_index { 0 };
  for (size_t image = 0; image < get_mask_names_count(input_file_mask, '#'); image++) {

    for (size_t i = 0; i < get_mask_names_count(input_file_mask, '#'); i++) {
      ppm.file = fopen(get_name_from_mask(input_file_mask, '#', view_index).c_str(), "rb");
      view_index++;
      if (ppm.file) {
        break;
      }
    }

    if (!ppm.file) {
      break;
    }

    readPPMHeader(&ppm);

    if ((ppm.width != width) || (ppm.height != height) || (ppm.color_depth != color_depth)) {
      cerr << "ERROR: PPMs DIMENSIONS MISMATCH" << endl;
      return 2;
    }

    for (size_t row = 0; row < ppm.height; row++) {
      if (readPPMRow(&ppm, ppm_row)) {
        cerr << "ERROR: BAD PPM" << endl;
        fclose(ppm.file);
        return 2;
      }

      for (size_t col = 0; col < ppm.width; col++) {
        rgb_data[(row * ppm.width + col) * 3 + 0] = ppm_row[col].r;
        rgb_data[(row * ppm.width + col) * 3 + 1] = ppm_row[col].g;
        rgb_data[(row * ppm.width + col) * 3 + 2] = ppm_row[col].b;
      }
    }

    fclose(ppm.file);

    uint8_t *inData[1]     = { &rgb_data[0] };
    int      inLinesize[1] = { static_cast<int>(3 * width) };

    sws_scale(in_convert_ctx, inData, inLinesize, 0, height, in_frame->data, in_frame->linesize);

    if (intra_only) {
      in_frame->pict_type = AV_PICTURE_TYPE_I;
    }

    in_frame->pts = image;

    encode(in_context, in_frame, pkt, savePkt);
  }

  encode(in_context, nullptr, pkt, savePkt);

  freePPMRow(ppm_row);
  avcodec_close(in_context);
  sws_freeContext(in_convert_ctx);
  avcodec_free_context(&in_context);
  av_frame_free(&in_frame);
  av_frame_free(&rgb_frame);
  av_packet_free(&pkt);

  return 0;
}
