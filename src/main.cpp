#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./image_convert input.jpg output.png\n";
        return 1;
    }

    const char* input_filename = argv[1];
    const char* output_filename = argv[2];

    avformat_network_init();

    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, input_filename, nullptr, nullptr) != 0) {
        std::cerr << "Failed to open input file\n";
        return 1;
    }

    std::cout << "Format: " << fmt_ctx->iformat->name << "\n";
    std::cout << "Number of streams: " << fmt_ctx->nb_streams << "\n";

    int ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Failed to get stream info: " << errbuf << "\n";
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    if (fmt_ctx->nb_streams == 0) {
        std::cerr << "No streams found in file\n";
        avformat_close_input(&fmt_ctx);
        return 1;
    }
    
    const AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
    if (!decoder) {
        std::cerr << "MJPEG decoder not found, trying JPEG decoder\n";
        decoder = avcodec_find_decoder_by_name("mjpeg");
    }
    if (!decoder) {
        std::cerr << "No JPEG decoder available\n";
        avformat_close_input(&fmt_ctx);
        return 1;
    }
    
    std::cout << "Using decoder: " << decoder->name << "\n";
    
    int video_stream_index = 0;
    AVStream* video_stream = fmt_ctx->streams[video_stream_index];
    
    AVCodecContext* dec_ctx = avcodec_alloc_context3(decoder);
    if (!dec_ctx) {
        std::cerr << "Failed to allocate decoder context\n";
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    dec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    dec_ctx->codec_id = decoder->id;
    
    ret = avcodec_open2(dec_ctx, decoder, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Failed to open decoder: " << errbuf << "\n";
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    bool got_frame = false;
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            ret = avcodec_send_packet(dec_ctx, pkt);
            if (ret == 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == 0) {
                    got_frame = true;
                    std::cout << "Decoded frame dimensions: " << frame->width << "x" << frame->height << "\n";
                    std::cout << "Pixel format: " << av_get_pix_fmt_name((AVPixelFormat)frame->format) << "\n";
                    break;
                }
            }
        }
        av_packet_unref(pkt);
    }

    if (!got_frame) {
        std::cerr << "Failed to decode any frame\n";
        av_frame_free(&frame);
        av_packet_free(&pkt);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Cleanup
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);

    return 0;
}