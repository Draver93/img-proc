#include "DeinterlaceThreadProcNode.h"

#include <vector>
#include <thread>

namespace img_deinterlace {

    DeinterlaceThreadProcNode::DeinterlaceThreadProcNode() { }
    DeinterlaceThreadProcNode::~DeinterlaceThreadProcNode() { }

    void DeinterlaceThreadProcNode::blend(AVFrame* frame) {
        if (!frame || !frame->data[0]) {
            throw std::runtime_error("Invalid frame data");
        }

        int width = frame->width;
        int height = frame->height;

        if (width <= 0 || height <= 0) {
            throw std::runtime_error("Invalid frame dimensions");
        }

        std::vector<std::thread> planesThreads;

        for (int plane = 0; plane < m_PlaneCount; ++plane) {
            if (!frame->data[plane]) continue;
            
            uint8_t* data = frame->data[plane];
            int stride = frame->linesize[plane];

            if (stride <= 0) continue;

            planesThreads.emplace_back([data, stride, width, height]() {
                std::vector<std::thread> linesThreads;
                for (int y = 1; y < height; y += 2) {
                    uint8_t* curr = data + y * stride;
                    uint8_t* prev = data + (y - 1) * stride;

                    linesThreads.emplace_back([curr, prev, width, stride]() {
                        for (int x = 0; x < width && x < stride; ++x) {
                            curr[x] = (curr[x] + prev[x]) / 2;
                        }
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

    void DeinterlaceThreadProcNode::init(std::shared_ptr<const PipelineContext> context) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(context->pixelFormat);
        if (desc) {
           if (!(desc->flags & AV_PIX_FMT_FLAG_PLANAR)) m_PlaneCount = 1;
           else m_PlaneCount = desc->nb_components;
        }
    }

    std::unique_ptr<PipelinePacket> DeinterlaceThreadProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}