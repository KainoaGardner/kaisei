#pragma once

#include "backend/backend.h"
#include <map>

namespace kaisei::backend {

struct FramebufferData {
    uint32_t fbo;
    uint32_t colorTexture;
    uint32_t width;
    uint32_t height;
};

class OpenGLBackend : public Backend {
public:
    OpenGLBackend();
    ~OpenGLBackend() override;

    // Initialization
    bool initialize() override;
    void shutdown() override;

    // Shader management
    uint32_t compileShader(const std::string& source, uint32_t type) override;
    uint32_t linkProgram(uint32_t vertexShader, uint32_t fragmentShader) override;
    void useProgram(uint32_t program) override;
    void deleteProgram(uint32_t program) override;

    // Texture management
    uint32_t createTexture(uint32_t width, uint32_t height, const void* data = nullptr) override;
    uint32_t loadTextureFromFile(const std::string& filePath) override;
    void bindTexture(uint32_t texture, uint32_t slot = 0) override;
    void deleteTexture(uint32_t texture) override;
    void getTextureSize(uint32_t texture, uint32_t& width, uint32_t& height) override;

    // Framebuffer management
    uint32_t createFramebuffer(uint32_t width, uint32_t height) override;
    void bindFramebuffer(uint32_t framebuffer) override;
    void unbindFramebuffer() override;
    uint32_t getFramebufferTexture(uint32_t framebuffer) override;
    void deleteFramebuffer(uint32_t framebuffer) override;

    // Uniform management
    void setUniformFloat(uint32_t program, const std::string& name, float value) override;
    void setUniformInt(uint32_t program, const std::string& name, int value) override;
    void setUniformVec2(uint32_t program, const std::string& name, float x, float y) override;
    void setUniformVec3(uint32_t program, const std::string& name, float x, float y, float z) override;
    void setUniformVec4(uint32_t program, const std::string& name, float x, float y, float z, float w) override;

    // Rendering
    void clear(float r, float g, float b, float a) override;
    void drawFullscreenQuad() override;
    void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

private:
    void createFullscreenQuad();
    void destroyFullscreenQuad();

    uint32_t quadVAO_;
    uint32_t quadVBO_;

    std::map<uint32_t, FramebufferData> framebuffers_;
    std::map<uint32_t, std::pair<uint32_t, uint32_t>> textureSizes_; // texture ID -> (width, height)

    bool initialized_;
    bool isGLES_;
};

} // namespace kaisei::backend
