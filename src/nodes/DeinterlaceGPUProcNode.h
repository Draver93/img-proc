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

#include <unordered_map>

namespace img_deinterlace {

    struct PairHash {
        template <typename T1, typename T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            std::size_t h1 = std::hash<T1>()(p.first);
            std::size_t h2 = std::hash<T2>()(p.second);
            return h1 ^ (h2 << 1); // or use boost::hash_combine style
        }
    };

    class DeinterlaceGPUProcNode : public Processor {
    public:
        DeinterlaceGPUProcNode();
        ~DeinterlaceGPUProcNode();
        
    private:
        GLuint m_ShaderProgram;
        std::unordered_map<std::pair<int, int>, GLuint, PairHash> m_InputTextures, m_OutputTextures;

        void compileShader();
        void createTextures(const AVPixFmtDescriptor *desc, const std::vector<int> &linesizes, int h);
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