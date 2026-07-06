#pragma once

#include "backend/openGLBackend.h"

namespace kaisei::integration::hyprland {

class HyprlandOpenGLBackend : public backend::OpenGLBackend {
public:
    HyprlandOpenGLBackend();
    ~HyprlandOpenGLBackend() override = default;

    void drawFullscreenQuad() override;
    void bindTexture(uint32_t texture, uint32_t slot = 0) override;
    void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

private:
    uint32_t boundTexture_;  // Texture bound to slot 0
    uint32_t viewportWidth_;
    uint32_t viewportHeight_;
};

} // namespace kaisei::integration::hyprland
