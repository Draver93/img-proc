#include "DeinterlaceAsyncProcNode.h"

#include <algorithm>
#include <future>
#include <functional>

namespace img_deinterlace {

    DeinterlaceAsyncProcNode::DeinterlaceAsyncProcNode() { }
    DeinterlaceAsyncProcNode::~DeinterlaceAsyncProcNode() { }

    void DeinterlaceAsyncProcNode::blend(AVFrame* frame) {
        int width = frame->width;
        int height = frame->height;

        std::vector<std::future<void>> planes_futures;
  
        for (int plane = 0; plane < 3; ++plane) {
            uint8_t* data = frame->data[plane];
            int stride = frame->linesize[plane];

            planes_futures.emplace_back(std::async(std::launch::async, [data, stride, width, height]() {
                std::vector<std::future<void>> lines_futures;
                for (int y = 1; y < height; y += 2) {
                    uint8_t* curr = data + y * stride;
                    uint8_t* prev = data + (y - 1) * stride;
                    lines_futures.emplace_back(std::async(std::launch::async, [curr, prev, width]() {
                        for (int x = 0; x < width; ++x) { curr[x] = (curr[x] + prev[x]) / 2; }
                    }));
                }
                for (auto& fut : lines_futures) fut.get();
            }));
        }
        for (auto& fut : planes_futures) fut.get();

    }

    std::unique_ptr<PipelinePacket> DeinterlaceAsyncProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}