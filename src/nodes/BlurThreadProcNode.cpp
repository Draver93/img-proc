#include "BlurThreadProcNode.h"

#include <vector>
#include <thread>


namespace media_proc {

    BlurThreadProcNode::BlurThreadProcNode() : m_Pool(std::thread::hardware_concurrency()) { }
    BlurThreadProcNode::~BlurThreadProcNode() { }

    void BlurThreadProcNode::blend(AVFrame* frame) {
        media_proc::Timer timer("Running blur with mode: threads");
        if (!frame || !frame->data[0]) throw std::runtime_error("Invalid frame data");
        
        int width = frame->width;
        int height = frame->height;
        if (width <= 0 || height <= 0) throw std::runtime_error("Invalid frame dimensions");
        
        // Simple 3x3 Gaussian kernel
        const float kernel[3][3] = {
            {1.0f/16, 2.0f/16, 1.0f/16},
            {2.0f/16, 4.0f/16, 2.0f/16},
            {1.0f/16, 2.0f/16, 1.0f/16}
        };
        
        for (int plane = 0; plane < m_PlaneCount; ++plane) {
            if (!frame->data[plane]) continue;
            
            uint8_t* data = frame->data[plane];
            int planeWidth = frame->linesize[plane];
            int planeHeight = (plane > 0 ? height >> m_Log2ChromaHeight : height);
            
            if (planeWidth <= 0 || planeHeight <= 2) continue;
            
            // Create temporary buffer for this plane - shared among threads
            auto tempBuffer = std::make_shared<std::vector<uint8_t>>(planeWidth * planeHeight);
            
            // Process each row (skip first and last row for border safety)
            for (int y = 1; y < planeHeight - 1; ++y) {
                // Capture necessary data for the thread
                m_Pool.enqueue([=, &kernel, tempBuffer]() {
                    uint8_t* curr = data + y * planeWidth;
                    uint8_t* prev = data + (y - 1) * planeWidth;
                    uint8_t* next = data + (y + 1) * planeWidth;
                    uint8_t* dst = tempBuffer->data() + y * planeWidth;
                    
                    // Apply Gaussian blur to this row (skip first and last pixel)
                    for (int x = 1; x < planeWidth - 1; ++x) {
                        float sum = 0.0f;
                        
                        // Apply 3x3 Gaussian kernel
                        for (int ky = -1; ky <= 1; ++ky) {
                            for (int kx = -1; kx <= 1; ++kx) {
                                uint8_t* row = (ky == -1) ? prev : (ky == 0) ? curr : next;
                                uint8_t pixel = row[x + kx];
                                sum += pixel * kernel[ky + 1][kx + 1];
                            }
                        }
                        
                        dst[x] = static_cast<uint8_t>(std::round(sum));
                    }
                });
            }
            
            // Wait for all blur operations to complete
            m_Pool.wait();
            
            // Copy blurred data back in a separate threading pass
            for (int y = 1; y < planeHeight - 1; ++y) {
                m_Pool.enqueue([=, tempBuffer]() {
                    uint8_t* src = tempBuffer->data() + y * planeWidth;
                    uint8_t* dst = data + y * planeWidth;
                    
                    // Copy the processed pixels (skip first and last pixel of each row)
                    std::memcpy(dst + 1, src + 1, planeWidth - 2);
                });
            }
            
            // Wait for all copy operations to complete
            m_Pool.wait();
        }
    }

    void BlurThreadProcNode::init(std::shared_ptr<const PipelineContext> context) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(context->pixelFormat);
        if (desc) {
           if (!(desc->flags & AV_PIX_FMT_FLAG_PLANAR)) m_PlaneCount = 1;
           else m_PlaneCount = desc->nb_components;
           m_Log2ChromaHeight = desc->log2_chroma_h;
        }
    }

    std::unique_ptr<PipelinePacket> BlurThreadProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}