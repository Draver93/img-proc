/*
 * GPU Deinterlace Processor Node
 * ==============================
 * 
 * GPU-accelerated deinterlacing implementation using OpenGL compute shaders.
 * Provides high-performance parallel processing on graphics hardware.
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 */

#ifndef IMG_DEINT_GPU_PROCESSOR_NODE_H
#define IMG_DEINT_GPU_PROCESSOR_NODE_H

#include "base/Processor.h"

namespace img_deinterlace {

    class DeinterlaceGPUProcNode : public Processor {
    public:
        DeinterlaceGPUProcNode();
        ~DeinterlaceGPUProcNode();
        
    private:
        GLuint m_ShaderProgram;
        GLuint m_InputTexture, m_OutputTexture;

        void compileShader();
        void createTextures(int w, int h);
        void blend(AVFrame* frame);

    private:
        virtual void init(std::shared_ptr<const PipelineContext> context) override;
        virtual std::unique_ptr<PipelinePacket> updatePacket(std::unique_ptr<PipelinePacket> packet) override;
                
    private:
        GLFWwindow* m_GLFWInstance = nullptr;
        const AVPixFmtDescriptor *m_PixelFormatDesc;

    };
}


#endif //!IMG_DEINT_GPU_PROCESSOR_NODE_H