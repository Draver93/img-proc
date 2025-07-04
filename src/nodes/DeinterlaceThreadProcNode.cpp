#include "DeinterlaceThreadProcNode.h"
#include "../timer/Timer.h"

#include <vector>
#include <thread>


namespace img_deinterlace {

    ThreadPool::ThreadPool(size_t numThreads) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this]() {
                            return stop || !tasks.empty();
                        });

                        if (stop && tasks.empty()) return;

                        task = std::move(tasks.front());
                        tasks.pop();
                        activeTasks++;
                    }
                    task();
                    activeTasks--;
                }
            });
        }
    }

    ThreadPool::~ThreadPool() {
        stop = true;
        condition.notify_all();
        for (std::thread &worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }

    void ThreadPool::enqueue(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            tasks.emplace(std::move(task));
        }
        condition.notify_one();
    }

    void ThreadPool::wait() {
        while (activeTasks > 0 || !tasks.empty()) {
            std::this_thread::yield();
        }
    }


    DeinterlaceThreadProcNode::DeinterlaceThreadProcNode() : m_Pool(std::thread::hardware_concurrency()) { }
    DeinterlaceThreadProcNode::~DeinterlaceThreadProcNode() { }

    void DeinterlaceThreadProcNode::blend(AVFrame* frame) {
        img_deinterlace::Timer timer("Running blend with mode: threads");

        if (!frame || !frame->data[0]) throw std::runtime_error("Invalid frame data");

        int width = frame->width;
        int height = frame->height;
        if (width <= 0 || height <= 0) throw std::runtime_error("Invalid frame dimensions");

        for (int plane = 0; plane < m_PlaneCount; ++plane) {
            if (!frame->data[plane]) continue;

            uint8_t* data = frame->data[plane];
            int planeWidth = frame->linesize[plane];
            int planeHeight = (plane > 0 ? height >> m_Log2ChromaHeight : height);
            if (planeWidth <= 0) continue;

            for (int y = 1; y < planeHeight; y += 2) {
                uint8_t* curr = data + y * planeWidth;
                uint8_t* prev = data + (y - 1) * planeWidth;

                m_Pool.enqueue([curr, prev, planeWidth]() {
                    for (int x = 0; x < planeWidth; ++x) {
                        curr[x] = (curr[x] + prev[x]) / 2;
                    }
                });
            }
        }

        m_Pool.wait(); // Wait for all lines to finish
    }

    void DeinterlaceThreadProcNode::init(std::shared_ptr<const PipelineContext> context) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(context->pixelFormat);
        if (desc) {
           if (!(desc->flags & AV_PIX_FMT_FLAG_PLANAR)) m_PlaneCount = 1;
           else m_PlaneCount = desc->nb_components;
           m_Log2ChromaHeight = desc->log2_chroma_h;
        }
    }

    std::unique_ptr<PipelinePacket> DeinterlaceThreadProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}