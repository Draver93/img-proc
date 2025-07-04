/*
 * Image Deinterlace Tool
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


#include "StdAfx.h"
#include "parser/CommandLineParser.h"

#include "nodes/FFmpegEncNode.h"
#include "nodes/FFmpegDecNode.h"

#include "nodes/DeinterlaceProcNode.h"
#include "nodes/DeinterlaceAsyncProcNode.h"
#include "nodes/DeinterlaceThreadProcNode.h"
#include "nodes/DeinterlaceGPUProcNode.h"


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
  --mode, -m      Processing mode: default, async, threads, gpu (Optional, default: default)
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
    else { 
        std::cerr << "Error: --input/-i is required (use --help/-h for more info)\n"; 
        return 1; 
    }

    std::string outputFilename = "output.jpeg";
    if (parser.hasOption("--output")) outputFilename = parser.getOption("--output");
    else if(parser.hasOption("-o")) outputFilename = parser.getOption("-o");

    std::string pipelineMode = parser.getOption("--mode", "default");
    if(pipelineMode == "default") pipelineMode = parser.getOption("-m", "default");

    std::unique_ptr<img_deinterlace::PipelineNode> rootNode;
    
    if(pipelineMode == "default") {
        rootNode = std::make_unique<img_deinterlace::FFmpegDecNode>(inputFilename);
        rootNode->setNext(std::make_unique<img_deinterlace::DeinterlaceProcNode>());
        rootNode->setNext(std::make_unique<img_deinterlace::FFmpegEncNode>(outputFilename));
    }
    else if(pipelineMode == "async") {
        rootNode = std::make_unique<img_deinterlace::FFmpegDecNode>(inputFilename);
        rootNode->setNext(std::make_unique<img_deinterlace::DeinterlaceAsyncProcNode>());
        rootNode->setNext(std::make_unique<img_deinterlace::FFmpegEncNode>(outputFilename));
    }
    else if(pipelineMode == "threads") {
        rootNode = std::make_unique<img_deinterlace::FFmpegDecNode>(inputFilename);
        rootNode->setNext(std::make_unique<img_deinterlace::DeinterlaceThreadProcNode>());
        rootNode->setNext(std::make_unique<img_deinterlace::FFmpegEncNode>(outputFilename));
    }
    else if(pipelineMode == "gpu") {
        rootNode = std::make_unique<img_deinterlace::FFmpegDecNode>(inputFilename);
        rootNode->setNext(std::make_unique<img_deinterlace::DeinterlaceGPUProcNode>());
        rootNode->setNext(std::make_unique<img_deinterlace::FFmpegEncNode>(outputFilename));
    }
    else { 
        std::cerr << "Error: --mode/-m should be one of: [default, async, threads, gpu]\n"; 
        return 1; 
    }

    rootNode->execute();
}