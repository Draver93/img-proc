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
        virtual std::unique_ptr<PipelinePacket> onPacket(std::unique_ptr<PipelinePacket> packet = nullptr) final override {
            if(!m_NodeInit) { init(); m_NodeInit = true; }
            
            packet = getPacket();
            if(!packet) m_EOS = true;

            return std::move(packet);
        }
        virtual bool isComplete() final override { 
            return m_EOS;
        };

    private:
        virtual void init() = 0;
        virtual std::unique_ptr<PipelinePacket> getPacket() = 0;

        bool m_EOS = false;
        bool m_NodeInit = false;
    };
}


#endif //!IMG_DEINT_DECODER_H