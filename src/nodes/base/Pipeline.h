/*
 * Pipeline Architecture
 * =====================
 * 
 * Implements the Chain of Responsibility pattern for processing image data
 * through a series of nodes (decoder -> processor -> encoder).
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 */

#ifndef IMG_DEINT_PIPELINE_H
#define IMG_DEINT_PIPELINE_H


#include <StdAfx.h>

namespace img_deinterlace {
    class PipelineContext {
    public:
        int width = 0, height = 0;
        AVPixelFormat pixelFormat;
        AVRational timeBase, aspectRatio, frameRate;

    public:
        PipelineContext(int width, int height, AVPixelFormat pixelFormat, AVRational timeBase, AVRational frameRate):
            width(width), height(height), pixelFormat(pixelFormat), timeBase(timeBase), frameRate(frameRate) {
            aspectRatio = { width, height };
        };
    };

    class PipelinePacket {
    public:
        AVFrame *frame;
        std::shared_ptr<const PipelineContext> context;

    public:
        PipelinePacket(AVFrame *frame, std::shared_ptr<const PipelineContext> ctx) : frame(frame), context(ctx) {}
    };

    //Chain of Responsibilities
    class PipelineNode {
    protected:
        std::unique_ptr<PipelineNode> m_NextNode;

    public:
        void setNext(std::unique_ptr<PipelineNode> nextNode) { 
            m_NextNode = std::move(nextNode); 
        }
        virtual std::unique_ptr<PipelinePacket> onPacket(std::unique_ptr<PipelinePacket> packet = nullptr) { 
            return nullptr; 
        };
        virtual bool isComplete() { return true; };

        void execute(std::unique_ptr<PipelinePacket> packet = nullptr) {
            do {
                packet = onPacket(packet ? std::move(packet) : nullptr);
                if (m_NextNode) m_NextNode->execute(std::move(packet));
            } while(!isComplete());
        }
    };
}


#endif //!IMG_DEINT_PIPELINE_H

