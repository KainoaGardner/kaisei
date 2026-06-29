#include "integration/hyprland/hyprlandRenderer.h"

#include <spdlog/spdlog.h>
#include "backend/openGLBackend.h"

namespace kaisei::integration::hyprland {

HyprlandRenderer::HyprlandRenderer(core::Registry& registry)
    : registry_(registry), enabled_(false) {
    backend_ = std::make_unique<backend::OpenGLBackend>();
    if (!backend_->initialize()) {
        throw std::runtime_error("Failed to initialize backend");
    }
}

HyprlandRenderer::~HyprlandRenderer() = default;

void HyprlandRenderer::loadPreset(const std::string& name) {
    if (!registry_.presets().hasPreset(name)) {
        throw std::runtime_error("Preset '" + name + "' not found");
    }

    const auto* preset = registry_.presets().getPreset(name);

    if (!renderer_) {
        renderer_ = std::make_unique<renderer::MultiPassRenderer>(*backend_, registry_.modules());
    }

    renderer_->loadPreset(*preset);
    currentPreset_ = name;

    spdlog::info("Loaded preset '{}' for Hyprland", name);
}

void HyprlandRenderer::setEnabled(bool enabled) {
    enabled_ = enabled;
    spdlog::info("Effects {}", enabled_ ? "enabled" : "disabled");
}

std::string HyprlandRenderer::getStatus() const {
    std::string status = "Hyprland Integration Status:\n";
    status += "  Preset: " + (currentPreset_.empty() ? "none" : currentPreset_) + "\n";
    status += "  Enabled: " + std::string(enabled_ ? "yes" : "no") + "\n";
    return status;
}

void HyprlandRenderer::render(uint32_t inputTexture, uint32_t outputFbo, uint32_t width, uint32_t height) {
    if (!enabled_ || !renderer_) {
        return;
    }

    renderer_->render(inputTexture, width, height, outputFbo);
}

} // namespace kaisei::integration::hyprland
