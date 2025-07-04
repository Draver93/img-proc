/*
 * Encoder Base Class
 * ==================
 * 
 * Template Method pattern implementation for image encoders.
 * Provides a common interface for different encoding strategies.
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 */

#ifndef IMG_DEINT_ENCODER_H
#define IMG_DEINT_ENCODER_H


#include <StdAfx.h>
#include "Pipeline.h"

namespace img_deinterlace {

    //Template Method
    class Encoder : public PipelineNode {
    public:
        virtual ~Encoder() = default;

    public:
        virtual std::unique_ptr<PipelinePacket> onPacket(std::unique_ptr<PipelinePacket> packet = nullptr) final {
            if(!m_NodeInit) { init(packet->context); m_NodeInit = true; }
            writePacket(std::move(packet));
            return nullptr;
        }

    private:
        virtual void init(std::shared_ptr<const PipelineContext> context) = 0;
        virtual void writePacket(std::unique_ptr<PipelinePacket> packet) = 0;

        bool m_NodeInit = false;
    };
}


#endif //!IMG_DEINT_ENCODER_H