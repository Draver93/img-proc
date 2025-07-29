#include "BlurAsyncProcNode.h"

#include <algorithm>
#include <future>
#include <functional>


namespace media_proc {

    BlurAsyncProcNode::BlurAsyncProcNode() { }
    BlurAsyncProcNode::~BlurAsyncProcNode() { }

    void BlurAsyncProcNode::blend(AVFrame* frame) {
        media_proc::Timer timer("Running blur with mode: async");
        if (!frame || !frame->data[0]) throw std::runtime_error("Invalid frame data");
        
        int width = frame->width;
        int height = frame->height;
        if (width <= 0 || height <= 0) throw std::runtime_error("Invalid frame dimensions");
        
        unsigned int numCores = std::thread::hardware_concurrency();
        if (numCores == 0) numCores = 4;
        
        // Simple 3x3 Gaussian kernel
        const float kernel[3][3] = {
            {1.0f/16, 2.0f/16, 1.0f/16},
            {2.0f/16, 4.0f/16, 2.0f/16},
            {1.0f/16, 2.0f/16, 1.0f/16}
        };
        
        std::vector<std::future<void>> planeFutures;
        
        for (int plane = 0; plane < m_PlaneCount; ++plane) {
            if (!frame->data[plane]) continue;
            
            uint8_t* data = frame->data[plane];
            int planeWidth = frame->linesize[plane];
            int planeHeight = (plane > 0 ? height >> m_Log2ChromaHeight : height);
            
            if (planeWidth <= 0 || planeHeight <= 0) continue;
            
            // Create temporary buffer for this plane
            std::vector<uint8_t> tempBuffer(planeWidth * planeHeight);
            
            planeFutures.emplace_back(std::async(std::launch::async, [=, &tempBuffer]() {
                std::vector<std::future<void>> chunkFutures;
                int chunkHeight = (planeHeight - 2) / numCores; // -2 to account for border
                
                for (unsigned int core = 0; core < numCores; ++core) {
                    int startY = 1 + core * chunkHeight;
                    int endY = (core + 1 == numCores) ? planeHeight - 1 : startY + chunkHeight;
                    
                    chunkFutures.emplace_back(std::async(std::launch::async, [=, &tempBuffer]() {
                        for (int y = startY; y < endY; ++y) {
                            for (int x = 1; x < planeWidth - 1; ++x) {
                                float sum = 0.0f;
                                
                                // Apply 3x3 Gaussian kernel
                                for (int ky = -1; ky <= 1; ++ky) {
                                    for (int kx = -1; kx <= 1; ++kx) {
                                        int srcY = y + ky;
                                        int srcX = x + kx;
                                        uint8_t pixel = data[srcY * planeWidth + srcX];
                                        sum += pixel * kernel[ky + 1][kx + 1];
                                    }
                                }
                                
                                tempBuffer[y * planeWidth + x] = static_cast<uint8_t>(std::round(sum));
                            }
                        }
                    }));
                }
                
                // Wait for all chunks to complete
                for (auto& cf : chunkFutures) cf.get();
                
                // Copy blurred data back (excluding borders)
                for (int y = 1; y < planeHeight - 1; ++y) {
                    for (int x = 1; x < planeWidth - 1; ++x) {
                        data[y * planeWidth + x] = tempBuffer[y * planeWidth + x];
                    }
                }
            }));
        }
        
        for (auto& pf : planeFutures) pf.get();
    }

    void BlurAsyncProcNode::init(std::shared_ptr<const PipelineContext> context) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(context->pixelFormat);
        if (desc) {
            if (!(desc->flags & AV_PIX_FMT_FLAG_PLANAR)) m_PlaneCount = 1;
            else m_PlaneCount = desc->nb_components;
            m_Log2ChromaHeight = desc->log2_chroma_h;
        }
    }

    std::unique_ptr<PipelinePacket> BlurAsyncProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}