#include "BlurGPUProcNode.h"

#include <vector>
#include <thread>

namespace media_proc {

    const char* blendShaderSource = R"(
        #version 430

        layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

        // Bindings for 8-bit input/output images
        layout(binding = 0, r8ui) uniform readonly uimage2D inputImage;
        layout(binding = 1, r8ui) uniform writeonly uimage2D outputImage;

        uniform int width;
        uniform int height;

        void main() {
            uint x = gl_GlobalInvocationID.x;
            uint y = gl_GlobalInvocationID.y;

            if (x >= uint(width) || y >= uint(height)) return;

            ivec2 coord = ivec2(x, y);

            if (y % 2 == 1) {
                // Read current and previous rows
                uint currValue = imageLoad(inputImage, coord).r;
                uint prevValue = imageLoad(inputImage, ivec2(x, y - 1)).r;

                // Blend and write result
                uint blended = (currValue + prevValue) / 2;
                imageStore(outputImage, coord, uvec4(blended, 0, 0, 0));
            } else {
                // Copy original pixel
                uint value = imageLoad(inputImage, coord).r;
                imageStore(outputImage, coord, uvec4(value, 0, 0, 0));
            }
        }
    )";

    BlurGPUProcNode::BlurGPUProcNode() { }
    BlurGPUProcNode::~BlurGPUProcNode() { 
        if (m_ShaderProgram) glDeleteProgram(m_ShaderProgram);
        for (auto &[key, texture] : m_InputTextures) glDeleteTextures(1, &texture);
        for (auto &[key, texture] : m_OutputTextures) glDeleteTextures(1, &texture);

        if (m_GLFWInstance) {
            glfwDestroyWindow(m_GLFWInstance);
            glfwTerminate();
            m_GLFWInstance = nullptr;
        }
    }

    void BlurGPUProcNode::compileShader() {
        GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(shader, 1, &blendShaderSource, nullptr);
        glCompileShader(shader);

        // Check for compilation errors
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            throw std::runtime_error("Error compiling shader: " + std::string(infoLog) );
        }

        m_ShaderProgram = glCreateProgram();
        glAttachShader(m_ShaderProgram, shader);
        glLinkProgram(m_ShaderProgram);
        glDeleteShader(shader);

        m_UniformWidthLoc = glGetUniformLocation(m_ShaderProgram, "width");
        m_UniformHeightLoc = glGetUniformLocation(m_ShaderProgram, "height");
    }

    void BlurGPUProcNode::createTextures(const AVPixFmtDescriptor *desc, const std::vector<int> &linesizes, int height) {
        for(int plane = 0; plane < desc->nb_components; plane++) {
            std::pair<int, int> textureSize = { linesizes[plane], (plane > 0 ? height >> m_PixelFormatDesc->log2_chroma_h : height) };

            glGenTextures(1, &m_InputTextures[textureSize]);
            glBindTexture(GL_TEXTURE_2D, m_InputTextures[textureSize]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, textureSize.first, textureSize.second);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureSize.first, textureSize.second, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

            glGenTextures(1, &m_OutputTextures[textureSize]);
            glBindTexture(GL_TEXTURE_2D, m_OutputTextures[textureSize]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, textureSize.first, textureSize.second);
        }
    }

    void BlurGPUProcNode::blend(AVFrame* frame) {
        img_deinterlace::Timer timer("Running blend with mode: GPU");

        if (!frame || !frame->data[0]) {
            throw std::runtime_error("Invalid frame data");
        }

        int width = frame->width;
        int height = frame->height;

        if (width <= 0 || height <= 0) {
            throw std::runtime_error("Invalid frame dimensions");
        }

        for (int plane = 0; plane < m_PixelFormatDesc->nb_components; ++plane) {
            if (!frame->data[plane]) continue;
            
            uint8_t* data = frame->data[plane];
            std::pair<int, int> textureSize = { frame->linesize[plane], (plane > 0 ? height >> m_PixelFormatDesc->log2_chroma_h : height) };

            if (textureSize.first <= 0 || textureSize.second <= 0) continue;

            glBindTexture(GL_TEXTURE_2D, m_InputTextures[textureSize]);
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, textureSize.first, textureSize.second, GL_RED_INTEGER, GL_UNSIGNED_BYTE, data );

            glBindImageTexture(0, m_InputTextures[textureSize], 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
            glBindImageTexture(1, m_OutputTextures[textureSize], 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);

            {
                glUseProgram(m_ShaderProgram);
 
                glUniform1i(m_UniformWidthLoc, textureSize.first );
                glUniform1i(m_UniformHeightLoc, textureSize.second );

                int workGroupX = (textureSize.first + 15) / 16;
                int workGroupY = (textureSize.second + 15) / 16;
                glDispatchCompute(workGroupX, workGroupY, 1);

                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            }

            glBindTexture(GL_TEXTURE_2D, m_OutputTextures[textureSize]);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, data);
        }
    }

    void BlurGPUProcNode::init(std::shared_ptr<const PipelineContext> context) {

        if (!glfwInit()) { throw std::runtime_error("GLFW initialization failed!"); }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        m_GLFWInstance = glfwCreateWindow(1, 1, "BlurGPUProcNode", nullptr, nullptr);
        if (!m_GLFWInstance) {
            glfwTerminate();
            throw std::runtime_error("GLFW window creation failed!");
        }

        glfwMakeContextCurrent(m_GLFWInstance);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) throw std::runtime_error("Failed to initialize GLAD!" );

        const char* glslVersionStr = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
        if (!glslVersionStr) {
            throw std::runtime_error("Failed to get OpenGL Shading Language version");
        }
        
        try {
            if(std::stod(glslVersionStr) < 4.3) {
                throw std::runtime_error("OpenGL Shading Language requires version 4.3 or higher! Your current GLSL version is: " + 
                    std::string(glslVersionStr));
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to parse OpenGL Shading Language version: " + std::string(glslVersionStr));
        }

        m_PixelFormatDesc = av_pix_fmt_desc_get(context->pixelFormat);
        if (!m_PixelFormatDesc) throw std::runtime_error("Pixel Format Descriptor not found");

        compileShader();
        createTextures(m_PixelFormatDesc, context->linesizes, context->height);
    }

    std::unique_ptr<PipelinePacket> BlurGPUProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}