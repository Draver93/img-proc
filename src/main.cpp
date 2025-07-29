/*
 * Image Blur Tool
 * ======================
 * 
 * A high-performance C++ tool for deinterlacing interlaced images using
 * multiple processing modes including CPU, GPU, and multi-threaded approaches.
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 * 
 * Description:
 * Entry point to the tool that processes an interlaced image and outputs 
 * a deinterlaced version using a pipeline architecture.
 * 
 * Alternative FFmpeg solution:
 * ffmpeg -i interlaced.jpg -frames:v 1 -filter_complex 
 * "[0:v]yadif=mode=0[yadif_out];[yadif_out]mcdeint=mode=extra_slow[mcdeint_out];
 * [mcdeint_out]qp=10[result]" -map [result] deinterlaced.jpg
 */

 //#define TRACK_MEMORY

#include "StdAfx.h"
#include "parser/CommandLineParser.h"

#include "nodes/FFmpegEncNode.h"
#include "nodes/FFmpegDecNode.h"

#include "nodes/BlurProcNode.h"
#include "nodes/BlurAsyncProcNode.h"
#include "nodes/BlurThreadProcNode.h"
#include "nodes/BlurGPUProcNode.h"
#include "nodes/BlurSIMDProcNode.h"

void printHelp() {
    std::cout << R"(Image Blur Tool

Usage:
  img_blur --input <input_file> [--output <output_file>] [--mode <mode>]
  img_blur -i <input_file> [-o <output_file>] [-m <mode>]

Description:
  This tool applies a blur effect to an image.

Options:
  --input, -i     Path to the input image file. (Required)
  --output, -o    Path to save the output image file. (Optional, default: output.${input ext})
  --mode, -m      Processing mode to use. (Optional, default: default)
                  Available modes: default, async, threads, gpu, simd
  --help, -h      Show this help message and exit.

Processing Modes:
  default         Standard CPU processing (single-threaded)
  async           Asynchronous processing with background threads
  threads         Multi-threaded processing 
  gpu             GPU-accelerated processing using OpenGL/OpenCL
  simd            SIMD-optimized processing using CPU vector instructions

Example:
  img_blur --input photo.jpeg --output photo_blurred.jpeg --mode simd
  img_blur -i photo.jpg -m gpu
  img_blur --input original.png --mode threads
)";
}

int main(int argc, char* argv[]) {
    img_deinterlace::CommandLineParser parser(argc, argv);
    if (parser.getOptCount() == 0 || parser.hasOption("--help") || parser.hasOption("-h")) { printHelp(); return 0; }

    std::string inputFilename;
    if (parser.hasOption("--input")) inputFilename = parser.getOption("--input");
    else if(parser.hasOption("-i")) inputFilename = parser.getOption("-i");
    else { 
        std::cerr << "Error: --input/-i is required (use --help/-h for more info)\n"; 
        return 1; 
    }

    std::string outputFilename = "output" + inputFilename.substr(inputFilename.find_last_of('.'));
    if (parser.hasOption("--output")) outputFilename = parser.getOption("--output");
    else if(parser.hasOption("-o")) outputFilename = parser.getOption("-o");

    std::string pipelineMode = parser.getOption("--mode", "default");
    if(pipelineMode == "default") pipelineMode = parser.getOption("-m", "default");

    std::unique_ptr<img_deinterlace::PipelineNode> rootNode = nullptr;
    
    if(pipelineMode == "default") {
      img_deinterlace::Timer timer("Running pipeline with mode: default");

      rootNode = std::make_unique<img_deinterlace::FFmpegDecNode>(inputFilename);
      std::unique_ptr<img_deinterlace::PipelineNode> processor = std::make_unique<img_deinterlace::BlurProcNode>();
      std::unique_ptr<img_deinterlace::PipelineNode> encoder = std::make_unique<img_deinterlace::FFmpegEncNode>(outputFilename);
      processor->setNext(std::move(encoder));
      rootNode->setNext(std::move(processor));
      rootNode->execute();
    }
    else if(pipelineMode == "async") {
        img_deinterlace::Timer timer("Running pipeline with mode: async");

        rootNode = std::make_unique<img_deinterlace::FFmpegDecNode>(inputFilename);
        std::unique_ptr<img_deinterlace::PipelineNode> processor = std::make_unique<img_deinterlace::BlurAsyncProcNode>();
        std::unique_ptr<img_deinterlace::PipelineNode> encoder = std::make_unique<img_deinterlace::FFmpegEncNode>(outputFilename);
        processor->setNext(std::move(encoder));
        rootNode->setNext(std::move(processor));
        rootNode->execute();
    }
    else if(pipelineMode == "threads") {
        img_deinterlace::Timer timer("Running pipeline with mode: threads");

        rootNode = std::make_unique<img_deinterlace::FFmpegDecNode>(inputFilename);
        std::unique_ptr<img_deinterlace::PipelineNode> processor = std::make_unique<img_deinterlace::BlurThreadProcNode>();
        std::unique_ptr<img_deinterlace::PipelineNode> encoder = std::make_unique<img_deinterlace::FFmpegEncNode>(outputFilename);
        processor->setNext(std::move(encoder));
        rootNode->setNext(std::move(processor));
        rootNode->execute();
    }
    else if(pipelineMode == "gpu") {
        img_deinterlace::Timer timer("Running pipeline with mode: gpu");

        rootNode = std::make_unique<img_deinterlace::FFmpegDecNode>(inputFilename);
        std::unique_ptr<img_deinterlace::PipelineNode> processor = std::make_unique<img_deinterlace::BlurGPUProcNode>();
        std::unique_ptr<img_deinterlace::PipelineNode> encoder = std::make_unique<img_deinterlace::FFmpegEncNode>(outputFilename);
        processor->setNext(std::move(encoder));
        rootNode->setNext(std::move(processor));
        rootNode->execute();
    }
    else if(pipelineMode == "simd") {
      #ifdef USE_SIMD
        img_deinterlace::Timer timer("Running pipeline with mode: SIMD");

        rootNode = std::make_unique<img_deinterlace::FFmpegDecNode>(inputFilename);
        std::unique_ptr<img_deinterlace::PipelineNode> processor = std::make_unique<img_deinterlace::BlurSIMDProcNode>();
        std::unique_ptr<img_deinterlace::PipelineNode> encoder = std::make_unique<img_deinterlace::FFmpegEncNode>(outputFilename);
        processor->setNext(std::move(encoder));
        rootNode->setNext(std::move(processor));
        rootNode->execute();
      #else 
        std::cerr << "Error: --mode/-m simd is not supported\n"; 
      #endif
    }
    else { 
        std::cerr << "Error: --mode/-m should be one of: [default, async, threads, gpu, simd]\n"; 
        return 1; 
    }
}