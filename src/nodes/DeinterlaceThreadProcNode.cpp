#include "DeinterlaceThreadProcNode.h"

#include <vector>
#include <thread>

namespace img_deinterlace {

    DeinterlaceThreadProcNode::DeinterlaceThreadProcNode() { }
    DeinterlaceThreadProcNode::~DeinterlaceThreadProcNode() { }

    void DeinterlaceThreadProcNode::blend(AVFrame* frame) {
        int width = frame->width;
        int height = frame->height;

        std::vector<std::thread> planesThreads;

        for (int plane = 0; plane < 3; ++plane) {
            uint8_t* data = frame->data[plane];
            int stride = frame->linesize[plane];

            planesThreads.emplace_back([data, stride, width, height]() {
                std::vector<std::thread> linesThreads;
                for (int y = 1; y < height; y += 2) {
                    uint8_t* curr = data + y * stride;
                    uint8_t* prev = data + (y - 1) * stride;

                    linesThreads.emplace_back([curr, prev, width]() {
                        for (int x = 0; x < width; ++x) curr[x] = (curr[x] + prev[x]) / 2;
                    });
                }

                for (auto& th : linesThreads) {
                    if (th.joinable()) th.join();
                }
            });
        }

        for (auto& th : planesThreads) {
            if (th.joinable()) th.join();
        }
    }

    std::unique_ptr<PipelinePacket> DeinterlaceThreadProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}