/*
 * Thread Deinterlace Processor Node
 * ==================================
 * 
 * Multi-threaded deinterlacing implementation using std::thread.
 * Provides explicit thread management for parallel processing.
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 */

#ifndef IMG_DEINT_THREAD_PROCESSOR_NODE_H
#define IMG_DEINT_THREAD_PROCESSOR_NODE_H


#include "base/Processor.h"

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>

namespace img_deinterlace {
    class ThreadPool {
    public:
        ThreadPool(size_t numThreads);
        ~ThreadPool();

        void enqueue(std::function<void()> task);
        void wait();

    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;

        std::mutex queueMutex;
        std::condition_variable condition;
        std::atomic<bool> stop{false};
        std::atomic<int> activeTasks{0};
    };


    class DeinterlaceThreadProcNode : public Processor {
    public:
        DeinterlaceThreadProcNode();
        ~DeinterlaceThreadProcNode();
        
    private:
        void blend(AVFrame* frame);

    private:
        virtual void init(std::shared_ptr<const PipelineContext> context) override;
        virtual std::unique_ptr<PipelinePacket> updatePacket(std::unique_ptr<PipelinePacket> packet) override;
                
    private:
        ThreadPool m_Pool;

        int m_PlaneCount = -1;
        int m_Log2ChromaHeight = 0;
    };
}


#endif //!IMG_DEINT_THREAD_PROCESSOR_NODE_H