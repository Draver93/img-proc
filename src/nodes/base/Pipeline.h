#ifndef IMG_DEINT_PIPELINE_H
#define IMG_DEINT_PIPELINE_H


#include <StdAfx.h>

namespace img_deinterlace {
    class PipelinePacket {
        AVFrame *frame;
    };

    //Chain of Responsibilities
    class PipelineNode {
    protected:
        std::shared_ptr<PipelineNode> m_NextNode;

    public:
        std::shared_ptr<PipelineNode> setNext(std::shared_ptr<PipelineNode> nextNode) { 
            m_NextNode = nextNode; 
            return m_NextNode; 
        }
        virtual std::unique_ptr<PipelinePacket> onPacket(std::unique_ptr<PipelinePacket> packet = nullptr) { 
            return nullptr; 
        };
        void execute(std::unique_ptr<PipelinePacket> packet = nullptr) {
            packet = onPacket(std::move(packet));
            if (m_NextNode) m_NextNode->execute(std::move(packet));
        }
    };
}


#endif //!IMG_DEINT_PIPELINE_H

