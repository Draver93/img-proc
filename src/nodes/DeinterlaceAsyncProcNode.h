/*
 * Async Deinterlace Processor Node
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

namespace img_deinterlace {

    class DeinterlaceAsyncProcNode : public Processor {
    public:
        DeinterlaceAsyncProcNode();
        ~DeinterlaceAsyncProcNode();
        
    private:
        void blend(AVFrame* frame);

    private:
        virtual void init(std::shared_ptr<const PipelineContext> context) override;
        virtual std::unique_ptr<PipelinePacket> updatePacket(std::unique_ptr<PipelinePacket> packet) override;
                        
    private:
        int m_PlaneCount = -1;
        
    };
}


#endif //!IMG_DEINT_ASYNC_PROCESSOR_NODE_H