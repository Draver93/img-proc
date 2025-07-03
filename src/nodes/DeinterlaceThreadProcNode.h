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
        virtual std::unique_ptr<PipelinePacket> updatePacket(std::unique_ptr<PipelinePacket> packet) override;
    };
}


#endif //!IMG_DEINT_THREAD_PROCESSOR_NODE_H