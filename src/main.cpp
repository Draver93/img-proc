//=============================================
// 
// Image Deinterlace Tool
//---------------------------------------------
//
//
// Author: Finoshkin Aleksei
//---------------------------------------------
// 
// Description:
//
// Entry point to the tool that processes an interlaced image and outputs a deinterlaced version.
// 
// Better solution:
// ffmpeg -i interlaced.jpg -frames:v 1 -filter_complex "[0:v]yadif=mode=0[yadif_out];[yadif_out]mcdeint=mode=extra_slow[mcdeint_out];[mcdeint_out]qp=10[result]" -map [result] deinterlaced.jpg
//=============================================


#include "StdAfx.h"
#include "parser/CommandLineParser.h"

#include "nodes/FFmpegEncNode.h"
#include "nodes/FFmpegDecNode.h"
#include "nodes/DeinterlaceProcNode.h"


void printHelp() {
    std::cout << R"(Image Deinterlace Tool

Usage:
  img_deinterlace --input <input_file> [--output <output_file>]
  img_deinterlace -i <input_file> [-o <output_file>]

Description:
  This tool processes an interlaced image and outputs a deinterlaced version.

Options:
  --input, -i     Path to the input image file. (Required)
  --output, -o    Path to save the output image file. (Optional, default: output.jpeg)
  --help, -h      Show this help message and exit.

Example:
  img_deinterlace --input video_frame.jpeg --output frame_fixed.jpeg
  img_deinterlace -i frame.jpg
)";
}

int main(int argc, char* argv[]) {
    img_deinterlace::CommandLineParser parser(argc, argv);
    if (parser.getOptCount() == 0 || parser.hasOption("--help") || parser.hasOption("-h")) { printHelp(); return 0; }

    std::string inputFilename;
    if (parser.hasOption("--input")) inputFilename = parser.getOption("--input");
    else if(parser.hasOption("-i")) inputFilename = parser.getOption("-i");
    else { std::cerr << "--input/-i is required(More info: --help/-h)\n"; return 1; }

    std::string outputFilename = "output.jpeg";
    if (parser.hasOption("--output")) outputFilename = parser.getOption("--output");
    else if(parser.hasOption("-o")) outputFilename = parser.getOption("-o");


    img_deinterlace::PipelineNode rootNode;
    rootNode.execute();
    std::string pipelineMode = parser.getOption("--mode", "default");
    if(pipelineMode == "default") pipelineMode = parser.getOption("-m", "default");

    if(pipelineMode == "default") {
        rootNode.setNext(std::make_unique<img_deinterlace::FFmpegDecNode>())->
            setNext(std::make_unique<img_deinterlace::DeinterlaceProcNode>())->
            setNext(std::make_unique<img_deinterlace::FFmpegEncNode>());
    }
    else if(pipelineMode == "threads") {
        rootNode.setNext(std::make_unique<img_deinterlace::FFmpegDecNode>())->
            setNext(std::make_unique<img_deinterlace::DeinterlaceProcNode>())->
            setNext(std::make_unique<img_deinterlace::FFmpegEncNode>());
    }
    else if(pipelineMode == "gpu") {
        rootNode.setNext(std::make_unique<img_deinterlace::FFmpegDecNode>())->
            setNext(std::make_unique<img_deinterlace::DeinterlaceProcNode>())->
            setNext(std::make_unique<img_deinterlace::FFmpegEncNode>());
    }
    else { std::cerr << "--mode/-m should be one of: [default, threads, gpu]\n"; return 1; }

    rootNode.execute();



    avformat_network_init();

    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, inputFilename.c_str(), nullptr, nullptr) != 0) {
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
}