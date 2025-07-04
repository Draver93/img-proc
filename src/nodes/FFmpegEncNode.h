/*
 * FFmpeg Encoder Node
 * ===================
 * 
 * FFmpeg-based image encoder implementation using libavcodec and libavformat.
 * Supports various output formats through FFmpeg's encoding capabilities.
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 */

#ifndef IMG_DEINT_FFMPEG_ENC_NODE_H
#define IMG_DEINT_FFMPEG_ENC_NODE_H


#include "base/Encoder.h"

namespace img_deinterlace {

    class FFmpegEncNode : public Encoder {
    public:
        FFmpegEncNode(const std::string &fileName) : m_FileName(fileName), m_Packet(av_packet_alloc()), m_File(fileName, std::ios::binary) { }
        ~FFmpegEncNode();
    private:
        virtual void init(std::shared_ptr<const PipelineContext> context) override;
        virtual void writePacket(std::unique_ptr<PipelinePacket> packet) override;

    private:
        std::string m_FileName;
        std::ofstream m_File;
        AVPacket* m_Packet = nullptr;
        AVFormatContext* m_FormatContext = nullptr;
        AVCodecContext* m_EncoderContext = nullptr;
    };

}


#endif //!IMG_DEINT_FFMPEG_ENC_NODE_H