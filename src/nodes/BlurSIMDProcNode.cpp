#include "BlurSIMDProcNode.h"

#ifdef USE_SIMD

#include <immintrin.h> // AVX2

namespace media_proc {

    BlurSIMDProcNode::BlurSIMDProcNode() { }
    BlurSIMDProcNode::~BlurSIMDProcNode() { 
        
    }

    void BlurSIMDProcNode::blend(AVFrame* frame) {
        img_deinterlace::Timer timer("Running blend with mode: SIMD");

        if (!frame || !frame->data[0]) 
            throw std::runtime_error("Invalid frame data");

        int width = frame->width;
        int height = frame->height;
        if (width <= 0 || height <= 0) 
            throw std::runtime_error("Invalid frame dimensions");

        constexpr int SIMD_WIDTH = 32; // AVX2 processes 32 uint8_t per vector

        for (int plane = 0; plane < m_PlaneCount; ++plane) {
            if (!frame->data[plane]) continue;

            uint8_t* data = frame->data[plane];
            int stride = frame->linesize[plane];
            int planeHeight = (plane > 0 ? height >> m_Log2ChromaHeight : height);
            int planeWidth = stride;

            if (stride <= 0) continue;

            for (int y = 1; y < planeHeight; y += 2) {
                uint8_t* curr = data + y * stride;
                uint8_t* prev = data + (y - 1) * stride;

                int x = 0;

                // Process 32 bytes (pixels) at a time
                for (; x <= width - SIMD_WIDTH && x <= planeWidth - SIMD_WIDTH; x += SIMD_WIDTH) {
                    __m256i currVec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(curr + x));
                    __m256i prevVec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(prev + x));
                    __m256i avgVec  = _mm256_avg_epu8(currVec, prevVec); // (a + b + 1) / 2
                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(curr + x), avgVec);
                }

                // Fallback for remaining pixels
                for (; x < width && x < planeWidth; ++x) {
                    curr[x] = (curr[x] + prev[x]) / 2;
                }
            }
        }
    }

    void BlurSIMDProcNode::init(std::shared_ptr<const PipelineContext> context) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(context->pixelFormat);
        if (desc) {
           if (!(desc->flags & AV_PIX_FMT_FLAG_PLANAR)) m_PlaneCount = 1;
           else m_PlaneCount = desc->nb_components;
           m_Log2ChromaHeight = desc->log2_chroma_h;
        }
    }

    std::unique_ptr<PipelinePacket> BlurSIMDProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}

#endif