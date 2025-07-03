#include "DeinterlaceGPUProcNode.h"

#include <vector>
#include <thread>

namespace img_deinterlace {

    const char* blendShaderSource = R"(
        #version 430

        layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

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

    DeinterlaceGPUProcNode::DeinterlaceGPUProcNode() { }
    DeinterlaceGPUProcNode::~DeinterlaceGPUProcNode() { 
        if (m_ShaderProgram) glDeleteProgram(m_ShaderProgram);
        if (m_InputTexture) glDeleteTextures(1, &m_InputTexture);
        if (m_OutputTexture) glDeleteTextures(1, &m_OutputTexture);
        if (m_GLFWInstance) {
            glfwDestroyWindow(m_GLFWInstance);
            glfwTerminate();
            m_GLFWInstance = nullptr;
        }
    }

    void DeinterlaceGPUProcNode::compileShader() {
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
    }

    void DeinterlaceGPUProcNode::createTextures(int w, int h) {
        glGenTextures(1, &m_InputTexture);
        glBindTexture(GL_TEXTURE_2D, m_InputTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, w, h);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
        glBindImageTexture(0, m_InputTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);

        glGenTextures(1, &m_OutputTexture);
        glBindTexture(GL_TEXTURE_2D, m_OutputTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, w, h);
        glBindImageTexture(1, m_OutputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);
    }

    void DeinterlaceGPUProcNode::blend(AVFrame* frame) {
        int width = frame->width;
        int height = frame->height;

        for (int plane = 0; plane < m_PixelFormatDesc->nb_components; ++plane) {
            uint8_t* data = frame->data[plane];
            width >>= m_PixelFormatDesc->log2_chroma_w;   
            height >>= m_PixelFormatDesc->log2_chroma_h;  

            glBindTexture(GL_TEXTURE_2D, m_InputTexture);
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, data );

            {
                glUseProgram(m_ShaderProgram);
 
                glUniform1i(glGetUniformLocation(m_ShaderProgram, "height"), height);
                glUniform1i(glGetUniformLocation(m_ShaderProgram, "width"), width);

                int workGroupX = (width + 31) / 32;
                int workGroupY = height;

                glDispatchCompute(workGroupX, workGroupY, 1);

                glMemoryBarrier(GL_ALL_BARRIER_BITS);
            }

            glBindTexture(GL_TEXTURE_2D, m_OutputTexture);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, data);
        }
    }

    void DeinterlaceGPUProcNode::init(std::shared_ptr<const PipelineContext> context) {

        if (!glfwInit()) { throw std::runtime_error("GLFW initialization failed!"); }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        m_GLFWInstance = glfwCreateWindow(1, 1, "DeinterlaceGPUProcNode", nullptr, nullptr);
        if (!m_GLFWInstance) {
            glfwTerminate();
            throw std::runtime_error("GLFW window creation failed!");
        }

        glfwMakeContextCurrent(m_GLFWInstance);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) throw std::runtime_error("Failed to initialize GLAD!" );

        const char* glslVersionStr = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
        if(std::stod(glslVersionStr) < 4.3) 
            throw std::runtime_error("OpenGL Shading Language requires version 4.3 or higher!\nYour current GLSL version is: " + 
                std::string(glslVersionStr));

        m_PixelFormatDesc = av_pix_fmt_desc_get(context->pixelFormat);
        if (!m_PixelFormatDesc) throw std::runtime_error("Pixel Format Descriptor not found");

        compileShader();

        createTextures(context->width, context->height);
    }

    std::unique_ptr<PipelinePacket> DeinterlaceGPUProcNode::updatePacket(std::unique_ptr<PipelinePacket> packet) {
        if(packet) blend(packet->frame);
        return std::move(packet);
    };
}