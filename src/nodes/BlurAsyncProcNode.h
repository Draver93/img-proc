/*
 * Async Blur Processor Node
 * =================================
 * 
 * Asynchronous deinterlacing implementation using std::async and std::future.
 * Provides parallel processing capabilities for improved performance.
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 */

#ifndef IMG_DEINT_ASYNC_PROCESSOR_NODE_H
#define IMG_DEINT_ASYNC_PROCESSOR_NODE_H


#include "base/Processor.h"

namespace media_proc {

    class BlurAsyncProcNode : public Processor {
    public:
        BlurAsyncProcNode();
        ~BlurAsyncProcNode();
        
    private:
        void blend(AVFrame* frame);

    private:
        virtual void init(std::shared_ptr<const PipelineContext> context) override;
        virtual std::unique_ptr<PipelinePacket> updatePacket(std::unique_ptr<PipelinePacket> packet) override;
                        
    private:
        int m_PlaneCount = -1;
        int m_Log2ChromaHeight = 0;
    };
}


#endif //!IMG_DEINT_ASYNC_PROCESSOR_NODE_H