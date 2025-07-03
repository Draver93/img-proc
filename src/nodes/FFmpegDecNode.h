#ifndef IMG_DEINT_FFMPEG_DEC_NODE_H
#define IMG_DEINT_FFMPEG_DEC_NODE_H


#include "base/Decoder.h"

namespace img_deinterlace {

    class FFmpegDecNode : public Decoder {
    public:
        FFmpegDecNode(const std::string &fileName) : m_FileName(fileName), m_Packet(av_packet_alloc()) { }
        ~FFmpegDecNode();
        
    private:
        virtual void init() override;
        virtual std::unique_ptr<PipelinePacket> getPacket() override;

    private:
        std::string m_FileName;

        AVPacket* m_Packet = nullptr;
        AVCodecContext* m_DecoderContext = nullptr;
        AVFormatContext* m_FormatContext = nullptr;
        std::shared_ptr<const PipelineContext> m_PipelineContext = nullptr;
    };
}


#endif //!IMG_DEINT_FFMPEG_DEC_NODE_H