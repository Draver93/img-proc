#include "FFmpegEncNode.h"

namespace img_deinterlace {

    FFmpegEncNode::~FFmpegEncNode() {
        av_packet_free(&m_Packet);
        avcodec_free_context(&m_EncoderContext);
        avformat_close_input(&m_FormatContext);
        if (m_File.is_open()) { m_File.close(); }
    }

    void FFmpegEncNode::init(std::shared_ptr<const PipelineContext> context) {

        avformat_alloc_output_context2(&m_FormatContext, nullptr, nullptr, m_FileName.c_str());
        if (!m_FormatContext) {
            throw std::runtime_error("Failed to detect output format\n");
        }

        AVCodecID codecId = m_FormatContext->oformat->video_codec; 
        const AVCodec* encoder = avcodec_find_encoder(codecId);
        if (!encoder) throw std::runtime_error("Encoder not found");

        m_EncoderContext = avcodec_alloc_context3(encoder);
        m_EncoderContext->width = context->width;
        m_EncoderContext->height = context->height;
        m_EncoderContext->coded_width = context->width;
        m_EncoderContext->coded_height = context->height;
        m_EncoderContext->sample_aspect_ratio = context->aspectRatio;

        m_EncoderContext->pix_fmt = context->pixelFormat;
        m_EncoderContext->time_base  = {1, 25};

        if (m_FormatContext->oformat->flags & AVFMT_GLOBALHEADER)
            m_EncoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        int ret = avcodec_open2(m_EncoderContext, encoder, nullptr);
        if (ret < 0) {
            char errbuf[256]; av_strerror(ret, errbuf, sizeof(errbuf));
            throw std::runtime_error(std::string("Failed to open encoder: ") + errbuf);
        }

        // Create output stream
        AVStream* outputStream = avformat_new_stream(m_FormatContext, encoder);
        if (!outputStream) { throw std::runtime_error("Could not create output stream"); }

        avcodec_parameters_from_context(outputStream->codecpar, m_EncoderContext);
    }
    
    void FFmpegEncNode::writePacket(std::unique_ptr<PipelinePacket> packet) {
        if(!packet) return; 

        int ret = avcodec_send_frame(m_EncoderContext, packet->frame);
        if (ret < 0) { throw std::runtime_error("Error sending a frame for encoding\n"); }
    
        while (ret >= 0) {
            ret = avcodec_receive_packet(m_EncoderContext, m_Packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return;
            else if (ret < 0) { throw std::runtime_error("Error during encoding\n"); }
    
            m_File.write(reinterpret_cast<const char*>(m_Packet->data), m_Packet->size);
            av_packet_unref(m_Packet);
        }
    }
}