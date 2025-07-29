/*
 * Thread Blur Processor Node
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
#include <cstring>
#include <functional>
#include <condition_variable>

namespace media_proc {

    class ThreadPool {
    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        mutable std::mutex queueMutex;
        std::condition_variable condition;
        std::condition_variable finished;
        std::atomic<size_t> activeTasks{0};
        std::atomic<bool> stop{false};

    public:
        ThreadPool(size_t numThreads) {
            for (size_t i = 0; i < numThreads; ++i) {
                workers.emplace_back([this]() {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queueMutex);
                            condition.wait(lock, [this]() {
                                return stop.load() || !tasks.empty();
                            });
                            
                            if (stop.load() && tasks.empty()) {
                                return;
                            }
                            
                            task = std::move(tasks.front());
                            tasks.pop();
                            activeTasks.fetch_add(1);
                        }
                        
                        // Execute task outside the lock
                        try {
                            task();
                        } catch (...) {
                            // Handle exceptions to prevent thread termination
                            // Log error or handle as appropriate for your application
                        }
                        
                        // Notify completion
                        size_t remaining = activeTasks.fetch_sub(1) - 1;
                        if (remaining == 0) {
                            finished.notify_all();
                        }
                    }
                });
            }
        }

        ~ThreadPool() {
            stop.store(true);
            condition.notify_all();
            
            for (std::thread &worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }

        template<typename F>
        void enqueue(F&& task) {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (stop.load()) {
                    throw std::runtime_error("ThreadPool is stopped");
                }
                tasks.emplace(std::forward<F>(task));
            }
            condition.notify_one();
        }

        void wait() {
            std::unique_lock<std::mutex> lock(queueMutex);
            finished.wait(lock, [this]() {
                return tasks.empty() && activeTasks.load() == 0;
            });
        }
        
        size_t size() const {
            return workers.size();
        }
        
        bool empty() const {
            std::lock_guard<std::mutex> lock(queueMutex);
            return tasks.empty() && activeTasks.load() == 0;
        }
    };


    class BlurThreadProcNode : public Processor {
    public:
        BlurThreadProcNode();
        ~BlurThreadProcNode();
        
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