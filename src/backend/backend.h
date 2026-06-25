#pragma once

#include <cstdint>
#include <string>

namespace kaisei::backend {

class Backend {
public:
    virtual ~Backend() = default;

    // Initialization
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    // Shader management
    virtual uint32_t compileShader(const std::string& source, uint32_t type) = 0;
    virtual uint32_t linkProgram(uint32_t vertexShader, uint32_t fragmentShader) = 0;
    virtual void useProgram(uint32_t program) = 0;
    virtual void deleteProgram(uint32_t program) = 0;

    // Texture management
    virtual uint32_t createTexture(uint32_t width, uint32_t height, const void* data = nullptr) = 0;
    virtual uint32_t loadTextureFromFile(const std::string& filePath) = 0;
    virtual void bindTexture(uint32_t texture, uint32_t slot = 0) = 0;
    virtual void deleteTexture(uint32_t texture) = 0;
    virtual void getTextureSize(uint32_t texture, uint32_t& width, uint32_t& height) = 0;

    // Framebuffer management
    virtual uint32_t createFramebuffer(uint32_t width, uint32_t height) = 0;
    virtual void bindFramebuffer(uint32_t framebuffer) = 0;
    virtual void unbindFramebuffer() = 0;
    virtual uint32_t getFramebufferTexture(uint32_t framebuffer) = 0;
    virtual void deleteFramebuffer(uint32_t framebuffer) = 0;

    // Uniform management
    virtual void setUniformFloat(uint32_t program, const std::string& name, float value) = 0;
    virtual void setUniformInt(uint32_t program, const std::string& name, int value) = 0;
    virtual void setUniformVec2(uint32_t program, const std::string& name, float x, float y) = 0;
    virtual void setUniformVec3(uint32_t program, const std::string& name, float x, float y, float z) = 0;
    virtual void setUniformVec4(uint32_t program, const std::string& name, float x, float y, float z, float w) = 0;

    // Rendering
    virtual void clear(float r, float g, float b, float a) = 0;
    virtual void drawFullscreenQuad() = 0;
    virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

    // Shader types (to be used with compileShader)
    enum ShaderType {
        VERTEX_SHADER,
        FRAGMENT_SHADER
    };
};

} // namespace kaisei::backend
