#include "integration/hyprland/hyprlandRenderer.h"

#include <GLES3/gl32.h>
#include <spdlog/spdlog.h>
#include "backend/openGLBackend.h"
#include "utils/format.h"

namespace kaisei::integration::hyprland {

HyprlandRenderer::HyprlandRenderer(core::Registry& registry)
    : registry_(registry), enabled_(false), tempFbo_(0), tempWidth_(0), tempHeight_(0) {
    backend_ = std::make_unique<backend::OpenGLBackend>();
    if (!backend_->initialize()) {
        throw std::runtime_error("Failed to initialize backend");
    }
}

HyprlandRenderer::~HyprlandRenderer() {
    if (tempFbo_ != 0) {
        backend_->deleteFramebuffer(tempFbo_);
    }
}

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
    if (enabled_ == enabled) {
        return;
    }

    enabled_ = enabled;

    if (!enabled && renderer_) {
        renderer_->cleanup();
    }

    spdlog::info("Effects {}", enabled_ ? "enabled" : "disabled");
}

std::string HyprlandRenderer::getStatus() const {
    std::string status = utils::bold("Hyprland Integration Status:\n");
    status += utils::bold("  Preset: ") + (currentPreset_.empty() ? "none" : currentPreset_) + "\n";
    status += utils::bold("  Enabled: ") + std::string(enabled_ ? "yes" : "no") + "\n";
    return status;
}

void HyprlandRenderer::render(uint32_t inputTexture, uint32_t outputFbo, uint32_t width, uint32_t height) {
    if (!enabled_ || !renderer_) {
        return;
    }

    if (tempFbo_ == 0 || tempWidth_ != width || tempHeight_ != height) {
        if (tempFbo_ != 0) {
            backend_->deleteFramebuffer(tempFbo_);
        }
        tempFbo_ = backend_->createFramebuffer(width, height);
        tempWidth_ = width;
        tempHeight_ = height;
    }

    renderer_->render(inputTexture, width, height, tempFbo_);

    uint32_t resultTexture = backend_->getFramebufferTexture(tempFbo_);

    backend_->bindFramebuffer(outputFbo);
    backend_->setViewport(0, 0, width, height);

    uint32_t passthroughProg = renderer_->getPassthroughProgram();
    backend_->useProgram(passthroughProg);
    backend_->bindTexture(resultTexture, 0);
    backend_->setUniformInt(passthroughProg, "u_inputTexture", 0);
    backend_->drawFullscreenQuad();
}

} // namespace kaisei::integration::hyprland
