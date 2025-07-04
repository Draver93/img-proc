/*
 * Processor Base Class
 * ====================
 * 
 * Template Method pattern implementation for image processors.
 * Provides a common interface for different processing strategies.
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 */

#ifndef IMG_DEINT_PROCESSOR_H
#define IMG_DEINT_PROCESSOR_H


#include <StdAfx.h>
#include "Pipeline.h"

namespace img_deinterlace {

    //Template Method
    class Processor : public PipelineNode {
    public:
        virtual ~Processor() = default;

    public:
        virtual std::unique_ptr<PipelinePacket> onPacket(std::unique_ptr<PipelinePacket> packet = nullptr) final {
            if(!m_NodeInit) { init(packet->context); m_NodeInit = true; }
            return updatePacket(std::move(packet));
        }

    private:
        virtual void init(std::shared_ptr<const PipelineContext> context) {};
        virtual std::unique_ptr<PipelinePacket> updatePacket(std::unique_ptr<PipelinePacket> packet) = 0;

        bool m_NodeInit = false;
    };
}


#endif //!IMG_DEINT_PROCESSOR_H