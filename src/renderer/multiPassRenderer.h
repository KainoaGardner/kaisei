#pragma once

#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "backend/backend.h"
#include "core/preset.h"
#include "core/moduleLoader.h"

namespace kaisei::renderer {

struct RenderPass {
    uint32_t program;
    std::string moduleName;
};

class MultiPassRenderer {
public:
    MultiPassRenderer(backend::Backend& backend, core::ModuleLoader& moduleLoader);
    ~MultiPassRenderer();

    void loadPreset(const core::Preset& preset);
    void cleanup();

    uint32_t render(uint32_t inputTexture, uint32_t width, uint32_t height, uint32_t outputFbo = 0);

    void setMousePosition(float x, float y) { mouseX_ = x; mouseY_ = y; }
    void setMouseButtons(bool left, bool right) { mouseLeftButton_ = left; mouseRightButton_ = right; }

    void reload();

    uint32_t getPassthroughProgram() const { return passthroughProgram_; }

private:
    void compileIfNeeded();
    void compilePass(const std::string& moduleName);
    void cleanupPasses();
    void cleanupFramebuffers();
    void setAutomaticUniforms(uint32_t program, uint32_t inputTexture, uint32_t width, uint32_t height,
                             const std::chrono::steady_clock::time_point& currentFrameTime);

    backend::Backend& backend_;
    core::ModuleLoader& moduleLoader_;

    std::vector<RenderPass> passes_;
    std::vector<uint32_t> framebuffers_;

    uint32_t vertexShader_;
    std::string vertexShaderSource_;
    uint32_t passthroughProgram_;

    const core::Preset* currentPreset_;
    bool needsCompilation_;

    // Automatic uniforms
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point lastFrameTime_;
    uint32_t frameCount_;
    float mouseX_;
    float mouseY_;
    bool mouseLeftButton_;
    bool mouseRightButton_;
    std::mt19937 randomEngine_;
    std::uniform_real_distribution<float> randomDist_;
};

} // namespace kaisei::renderer
