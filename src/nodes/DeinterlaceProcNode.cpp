#include "DeinterlaceProcNode.h"
#include "../timer/Timer.h"

namespace img_deinterlace {

    DeinterlaceProcNode::DeinterlaceProcNode() { }
    DeinterlaceProcNode::~DeinterlaceProcNode() { 
        
    }

    void DeinterlaceProcNode::blend(AVFrame* frame) {
        img_deinterlace::Timer timer("Running blend with mode: default");

        if (!frame || !frame->data[0])  throw std::runtime_error("Invalid frame data");

        int width = frame->width;
        int height = frame->height;
        if (width <= 0 || height <= 0) throw std::runtime_error("Invalid frame dimensions");

        for (int plane = 0; plane < m_PlaneCount; ++plane) {
            if (!frame->data[plane]) continue;
            
            uint8_t* data = frame->data[plane];
            int planeWidth = frame->linesize[plane];
            int planeHeight = (plane > 0 ? height >> m_Log2ChromaHeight : height);
            if (planeWidth <= 0) continue;

            for (int y = 1; y < height; y += 2) {
                uint8_t* curr = data + y * planeWidth;
                uint8_t* prev = data + (y - 1) * planeWidth;

                for (int x = 0; x < width && x < planeWidth; ++x) {
                    curr[x] = (curr[x] + prev[x]) / 2;
                }
            }
        }
    }

    void DeinterlaceProcNode::init(std::shared_ptr<const PipelineContext> context) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(context->pixelFormat);
        if (desc) {
           if (!(desc->flags & AV_PIX_FMT_FLAG_PLANAR)) m_PlaneCount = 1;
           else m_PlaneCount = desc->nb_components;
           m_Log2ChromaHeight = desc->log2_chroma_h;
        }
    }

    std::unique_ptr<PipelinePacket> DeinterlaceProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}