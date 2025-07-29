#include "BlurAsyncProcNode.h"

#include <algorithm>
#include <future>
#include <functional>


namespace media_proc {

    BlurAsyncProcNode::BlurAsyncProcNode() { }
    BlurAsyncProcNode::~BlurAsyncProcNode() { }

    void BlurAsyncProcNode::blend(AVFrame* frame) {
        media_proc::Timer timer("Running blend with mode: async");

        if (!frame || !frame->data[0]) throw std::runtime_error("Invalid frame data");

        int width = frame->width;
        int height = frame->height;
        if (width <= 0 || height <= 0) throw std::runtime_error("Invalid frame dimensions");

        unsigned int numCores = std::thread::hardware_concurrency();
        if (numCores == 0) numCores = 4;

        std::vector<std::future<void>> planeFutures;
        for (int plane = 0; plane < m_PlaneCount; ++plane) {
            if (!frame->data[plane]) continue;

            uint8_t* data = frame->data[plane];
            int planeWidth = frame->linesize[plane];
            int planeHeight = (plane > 0 ? height >> m_Log2ChromaHeight : height);
            if (planeWidth <= 0) continue;

            planeFutures.emplace_back(std::async(std::launch::async, [=]() {
                std::vector<std::future<void>> chunkFutures;
                int chunkHeight = planeHeight / numCores;

                for (unsigned int core = 0; core < numCores; ++core) {
                    int startY = 1 + core * chunkHeight;
                    int endY = (core + 1 == numCores) ? planeHeight : (core + 1) * chunkHeight;

                    chunkFutures.emplace_back(std::async(std::launch::async, [=]() {
                        for (int y = startY; y < endY; y += 2) {
                            uint8_t* curr = data + y * planeWidth;
                            uint8_t* prev = data + (y - 1) * planeWidth;
                            for (int x = 0; x < planeWidth; ++x) {
                                curr[x] = (curr[x] + prev[x]) / 2;
                            }
                        }
                    }));
                }

                for (auto& cf : chunkFutures) cf.get();
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