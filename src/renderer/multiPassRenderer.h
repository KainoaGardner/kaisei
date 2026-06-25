#pragma once

#include <memory>
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

    uint32_t render(uint32_t inputTexture, uint32_t width, uint32_t height);

    void reload();

private:
    void compilePass(const std::string& moduleName);
    void cleanupPasses();
    void cleanupFramebuffers();

    backend::Backend& backend_;
    core::ModuleLoader& moduleLoader_;

    std::vector<RenderPass> passes_;
    std::vector<uint32_t> framebuffers_;

    uint32_t vertexShader_;
    std::string vertexShaderSource_;
    uint32_t passthroughProgram_;

    const core::Preset* currentPreset_;
};

} // namespace kaisei::renderer
