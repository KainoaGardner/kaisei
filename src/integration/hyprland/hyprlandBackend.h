#pragma once

#include "backend/openGLBackend.h"

namespace kaisei::integration::hyprland {

class HyprlandOpenGLBackend : public backend::OpenGLBackend {
public:
    HyprlandOpenGLBackend() = default;
    ~HyprlandOpenGLBackend() override = default;

    void drawFullscreenQuad() override;

    bool initialize() override;
};

} // namespace kaisei::integration::hyprland
