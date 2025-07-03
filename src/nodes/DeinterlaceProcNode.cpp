#include "DeinterlaceProcNode.h"

namespace img_deinterlace {

    DeinterlaceProcNode::DeinterlaceProcNode() { }
    DeinterlaceProcNode::~DeinterlaceProcNode() { 
        
    }

    void DeinterlaceProcNode::blend(AVFrame* frame) {
        int width = frame->width;
        int height = frame->height;

        for (int plane = 0; plane < 3; ++plane) {
            uint8_t* data = frame->data[plane];
            int stride = frame->linesize[plane];

            for (int y = 1; y < height; y += 2) {
                uint8_t* curr = data + y * stride;
                uint8_t* prev = data + (y - 1) * stride;

                for (int x = 0; x < width; ++x) {
                    curr[x] = (curr[x] + prev[x]) / 2;
                }
            }
        }
    }

    std::unique_ptr<PipelinePacket> DeinterlaceProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}