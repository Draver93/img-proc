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

namespace img_deinterlace {

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
        int m_PlaneCount = -1;
        
    };
}


#endif //!IMG_DEINT_THREAD_PROCESSOR_NODE_H