/*
 * Deinterlace Processor Node
 * ==========================
 * 
 * CPU-based deinterlacing implementation using simple line blending.
 * Processes interlaced images by blending odd and even scan lines.
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 */

#ifndef IMG_DEINT_PROCESSOR_NODE_H
#define IMG_DEINT_PROCESSOR_NODE_H


#include "base/Processor.h"

namespace img_deinterlace {

    class DeinterlaceProcNode : public Processor {
    public:
        DeinterlaceProcNode();
        ~DeinterlaceProcNode();
        
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


#endif //!IMG_DEINT_PROCESSOR_NODE_H