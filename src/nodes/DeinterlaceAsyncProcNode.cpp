#include "DeinterlaceAsyncProcNode.h"

#include <algorithm>
#include <future>
#include <functional>

namespace img_deinterlace {

    DeinterlaceAsyncProcNode::DeinterlaceAsyncProcNode() { }
    DeinterlaceAsyncProcNode::~DeinterlaceAsyncProcNode() { }

    void DeinterlaceAsyncProcNode::blend(AVFrame* frame) {
        if (!frame || !frame->data[0]) {
            throw std::runtime_error("Invalid frame data");
        }

        int width = frame->width;
        int height = frame->height;

        if (width <= 0 || height <= 0) {
            throw std::runtime_error("Invalid frame dimensions");
        }

        std::vector<std::future<void>> planes_futures;
  
        for (int plane = 0; plane < m_PlaneCount; ++plane) {
            if (!frame->data[plane]) continue;
            
            uint8_t* data = frame->data[plane];
            int stride = frame->linesize[plane];

            if (stride <= 0) continue;

            planes_futures.emplace_back(std::async(std::launch::async, [data, stride, width, height]() {
                std::vector<std::future<void>> lines_futures;
                for (int y = 1; y < height; y += 2) {
                    uint8_t* curr = data + y * stride;
                    uint8_t* prev = data + (y - 1) * stride;
                    lines_futures.emplace_back(std::async(std::launch::async, [curr, prev, width, stride]() {
                        for (int x = 0; x < width && x < stride; ++x) { 
                            curr[x] = (curr[x] + prev[x]) / 2; 
                        }
                    }));
                }
                for (auto& fut : lines_futures) fut.get();
            }));
        }
        for (auto& fut : planes_futures) fut.get();
    }

    void DeinterlaceAsyncProcNode::init(std::shared_ptr<const PipelineContext> context) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(context->pixelFormat);
        if (desc) {
           if (!(desc->flags & AV_PIX_FMT_FLAG_PLANAR)) m_PlaneCount = 1;
           else m_PlaneCount = desc->nb_components;
        }
    }

    std::unique_ptr<PipelinePacket> DeinterlaceAsyncProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}