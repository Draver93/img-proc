#ifndef IMG_DEINT_DECODER_H
#define IMG_DEINT_DECODER_H


#include <StdAfx.h>
#include "Pipeline.h"

namespace img_deinterlace {

    //Template Method
    class Decoder : public PipelineNode {
    public:
        virtual ~Decoder() = default;
        
    public:
        virtual std::unique_ptr<PipelinePacket> onPacket(std::unique_ptr<PipelinePacket> packet = nullptr) final {
            if(!m_NodeInit) { init(); m_NodeInit = true; }
            return getPacket();
        }

    private:
        virtual void init() = 0;
        virtual std::unique_ptr<PipelinePacket> getPacket() = 0;

        bool m_NodeInit = false;
    };
}


#endif //!IMG_DEINT_DECODER_H