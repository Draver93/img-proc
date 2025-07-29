#include "BlurSIMDProcNode.h"

#ifdef USE_SIMD

#include <immintrin.h> // AVX2

namespace media_proc {

    BlurSIMDProcNode::BlurSIMDProcNode() { }
    BlurSIMDProcNode::~BlurSIMDProcNode() { 
        
    }

    void BlurSIMDProcNode::blend(AVFrame* frame) {
        media_proc::Timer timer("Running blur with mode: SIMD");
        if (!frame || !frame->data[0])
            throw std::runtime_error("Invalid frame data");
        
        int width = frame->width;
        int height = frame->height;
        if (width <= 0 || height <= 0)
            throw std::runtime_error("Invalid frame dimensions");
        
        constexpr int SIMD_WIDTH = 32; // AVX2 processes 32 uint8_t per vector
        
        // Gaussian kernel weights scaled by 256 for integer math
        // [1/16, 2/16, 1/16; 2/16, 4/16, 2/16; 1/16, 2/16, 1/16] * 256
        const uint16_t kernelWeights[9] = {16, 32, 16, 32, 64, 32, 16, 32, 16};
        
        for (int plane = 0; plane < m_PlaneCount; ++plane) {
            if (!frame->data[plane]) continue;
            
            uint8_t* data = frame->data[plane];
            int stride = frame->linesize[plane];
            int planeHeight = (plane > 0 ? height >> m_Log2ChromaHeight : height);
            int planeWidth = stride;
            
            if (stride <= 0 || planeHeight <= 2) continue;
            
            // Create temporary buffer for this plane
            std::vector<uint8_t> tempBuffer(stride * planeHeight);
            
            // Process rows (skip first and last row to avoid bounds checking)
            for (int y = 1; y < planeHeight - 1; ++y) {
                uint8_t* curr = data + y * stride;
                uint8_t* prev = data + (y - 1) * stride;
                uint8_t* next = data + (y + 1) * stride;
                uint8_t* dst = tempBuffer.data() + y * stride;
                
                int x = 1; // Start at x=1 to skip left border
                
                // SIMD processing for bulk of the row
                for (; x <= width - SIMD_WIDTH - 1 && x <= planeWidth - SIMD_WIDTH - 1; x += SIMD_WIDTH) {
                    // Load 3x3 neighborhood for 32 pixels
                    __m256i sum_lo = _mm256_setzero_si256();
                    __m256i sum_hi = _mm256_setzero_si256();
                    
                    // Process 3x3 kernel
                    for (int ky = -1; ky <= 1; ++ky) {
                        uint8_t* row = (ky == -1) ? prev : (ky == 0) ? curr : next;
                        
                        for (int kx = -1; kx <= 1; ++kx) {
                            int kernelIdx = (ky + 1) * 3 + (kx + 1);
                            uint16_t weight = kernelWeights[kernelIdx];
                            
                            // Load 32 pixels
                            __m256i pixels = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(row + x + kx));
                            
                            // Convert to 16-bit for multiplication
                            __m256i pixels_lo = _mm256_unpacklo_epi8(pixels, _mm256_setzero_si256());
                            __m256i pixels_hi = _mm256_unpackhi_epi8(pixels, _mm256_setzero_si256());
                            
                            // Multiply by kernel weight
                            __m256i weight_vec = _mm256_set1_epi16(weight);
                            pixels_lo = _mm256_mullo_epi16(pixels_lo, weight_vec);
                            pixels_hi = _mm256_mullo_epi16(pixels_hi, weight_vec);
                            
                            // Accumulate
                            sum_lo = _mm256_add_epi16(sum_lo, pixels_lo);
                            sum_hi = _mm256_add_epi16(sum_hi, pixels_hi);
                        }
                    }
                    
                    // Divide by 256 (shift right by 8) and pack back to uint8_t
                    sum_lo = _mm256_srli_epi16(sum_lo, 8);
                    sum_hi = _mm256_srli_epi16(sum_hi, 8);
                    __m256i result = _mm256_packus_epi16(sum_lo, sum_hi);
                    
                    // Store result
                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + x), result);
                }
                
                // Fallback for remaining pixels
                for (; x < width - 1 && x < planeWidth - 1; ++x) {
                    uint32_t sum = 0;
                    
                    // Apply 3x3 Gaussian kernel
                    for (int ky = -1; ky <= 1; ++ky) {
                        for (int kx = -1; kx <= 1; ++kx) {
                            int kernelIdx = (ky + 1) * 3 + (kx + 1);
                            uint8_t* row = (ky == -1) ? prev : (ky == 0) ? curr : next;
                            sum += row[x + kx] * kernelWeights[kernelIdx];
                        }
                    }
                    
                    dst[x] = static_cast<uint8_t>(sum >> 8); // Divide by 256
                }
            }
            
            // Copy blurred data back (excluding borders)
            for (int y = 1; y < planeHeight - 1; ++y) {
                uint8_t* src = tempBuffer.data() + y * stride;
                uint8_t* dst = data + y * stride;
                
                // Copy the processed pixels (skip first and last pixel of each row)
                std::memcpy(dst + 1, src + 1, std::min(width, planeWidth) - 2);
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