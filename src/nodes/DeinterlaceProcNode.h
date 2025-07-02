#ifndef IMG_DEINT_PROCESSOR_NODE_H
#define IMG_DEINT_PROCESSOR_NODE_H


#include "base/Processor.h"

namespace img_deinterlace {

    class DeinterlaceProcNode : public Processor {
    public:
        DeinterlaceProcNode() { }
        ~DeinterlaceProcNode() { }
        
    private:
        virtual void init() override {

        };
        virtual std::unique_ptr<PipelinePacket> updatePacket(std::unique_ptr<PipelinePacket> packet) override {
            return std::move(packet);
        };
    };
}


#endif //!IMG_DEINT_PROCESSOR_NODE_H