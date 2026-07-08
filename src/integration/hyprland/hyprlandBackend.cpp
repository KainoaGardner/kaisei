#include "integration/hyprland/hyprlandBackend.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace kaisei::integration::hyprland {

bool HyprlandOpenGLBackend::initialize() {
    if (initialized_) {
        spdlog::warn("Hyprland OpenGL backend already initialized");
        return true;
    }

    if (!gladLoadGL()) {
        spdlog::error("Failed to initialize glad (OpenGL function loader)");
        return false;
    }

    const char* versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (versionStr == nullptr) {
        spdlog::error("No OpenGL context available");
        return false;
    }

    spdlog::info("OpenGL Version: {}", versionStr);
    spdlog::info("GLSL Version: {}", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    spdlog::info("Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    std::string version(versionStr);
    isGLES_ = (version.find("OpenGL ES") != std::string::npos);
    spdlog::info("Backend: OpenGL {} (Hyprland)", isGLES_ ? "ES" : "Desktop");

    initialized_ = true;
    spdlog::debug("Hyprland OpenGL backend initialized");
    return true;
}

void HyprlandOpenGLBackend::drawFullscreenQuad() {
    if (quadVAO_ == 0) {
        createFullscreenQuad();
    }

    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace kaisei::integration::hyprland
