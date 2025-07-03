#include "FFmpegDecNode.h"

namespace img_deinterlace {

    FFmpegDecNode::~FFmpegDecNode() {
        av_packet_free(&m_Packet);
        avcodec_free_context(&m_DecoderContext);
        avformat_close_input(&m_FormatContext);
    }

    void FFmpegDecNode::init() {
        avformat_network_init();
        
        if (avformat_open_input(&m_FormatContext, m_FileName.c_str(), nullptr, nullptr) != 0) {
            throw std::runtime_error("Failed to open input file\n");
        }

        int ret = avformat_find_stream_info(m_FormatContext, nullptr);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            avformat_close_input(&m_FormatContext);
            throw std::runtime_error("Failed to get stream info: " + std::string(errbuf) + "\n");
        }

        if (m_FormatContext->nb_streams == 0) {
            avformat_close_input(&m_FormatContext);
            throw std::runtime_error("No streams found in file\n");
        }
        
        const AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_MJPEG);

        if (!decoder) {
            avformat_close_input(&m_FormatContext);
            throw std::runtime_error("No JPEG decoder available\n");
        }
        
        m_DecoderContext = avcodec_alloc_context3(decoder);
        if (!m_DecoderContext) {
            avformat_close_input(&m_FormatContext);
            throw std::runtime_error("Failed to allocate decoder context\n");
        }

        m_DecoderContext->codec_type = AVMEDIA_TYPE_VIDEO;
        m_DecoderContext->codec_id = decoder->id;
        
        ret = avcodec_open2(m_DecoderContext, decoder, nullptr);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            avcodec_free_context(&m_DecoderContext);
            avformat_close_input(&m_FormatContext);
            throw std::runtime_error("Failed to open decoder: " + std::string(errbuf) + "\n");
        }


    }
    std::unique_ptr<PipelinePacket> FFmpegDecNode::getPacket() {
        int video_stream_index = 0;
        AVStream* video_stream = m_FormatContext->streams[video_stream_index];
        AVFrame *frame = av_frame_alloc();

        bool got_frame = false;
        if(av_read_frame(m_FormatContext, m_Packet) >= 0) {
            if (m_Packet->stream_index == video_stream_index) {
                int ret = avcodec_send_packet(m_DecoderContext, m_Packet);
                if (ret == 0) {
                    ret = avcodec_receive_frame(m_DecoderContext, frame);
                    if (ret == 0) { got_frame = true; }
                    else if(frame) av_freep(frame);
                }
            }
            av_packet_unref(m_Packet);
        }
        if(!got_frame) return nullptr;

        if(!m_PipelineContext) {
            m_PipelineContext = std::make_shared<PipelineContext>(
                frame->width,
                frame->height,
                m_DecoderContext->pix_fmt,
                m_DecoderContext->time_base,
                m_DecoderContext->framerate );
        }

        return std::make_unique<PipelinePacket>(frame, m_PipelineContext); 
    };

}