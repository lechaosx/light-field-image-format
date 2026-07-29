#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <memory>

namespace ffmpeg {

inline constexpr auto delete_packet = [](AVPacket *packet) noexcept {
  av_packet_free(&packet);
};
inline constexpr auto delete_frame = [](AVFrame *frame) noexcept {
  av_frame_free(&frame);
};
inline constexpr auto delete_codec_context =
    [](AVCodecContext *context) noexcept { avcodec_free_context(&context); };

using Packet = std::unique_ptr<AVPacket, decltype(delete_packet)>;
using Frame = std::unique_ptr<AVFrame, decltype(delete_frame)>;
using CodecContext =
    std::unique_ptr<AVCodecContext, decltype(delete_codec_context)>;
using Parser =
    std::unique_ptr<AVCodecParserContext, decltype(&av_parser_close)>;
using ScaleContext = std::unique_ptr<SwsContext, decltype(&sws_freeContext)>;

}
