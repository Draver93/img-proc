#include "DeinterlaceProcNode.h"

namespace img_deinterlace {

    DeinterlaceProcNode::DeinterlaceProcNode() { }
    DeinterlaceProcNode::~DeinterlaceProcNode() { 
        
    }

    void DeinterlaceProcNode::blend(AVFrame* frame) {
        if (!frame || !frame->data[0]) {
            throw std::runtime_error("Invalid frame data");
        }

        int width = frame->width;
        int height = frame->height;

        if (width <= 0 || height <= 0) {
            throw std::runtime_error("Invalid frame dimensions");
        }

        for (int plane = 0; plane < m_PlaneCount; ++plane) {
            if (!frame->data[plane]) continue;
            
            uint8_t* data = frame->data[plane];
            int stride = frame->linesize[plane];

            if (stride <= 0) continue;

            for (int y = 1; y < height; y += 2) {
                uint8_t* curr = data + y * stride;
                uint8_t* prev = data + (y - 1) * stride;

                for (int x = 0; x < width && x < stride; ++x) {
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
        }
    }

    std::unique_ptr<PipelinePacket> DeinterlaceProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}