#pragma once

#include "backend/openGLBackend.h"

namespace kaisei::integration::hyprland {

class HyprlandOpenGLBackend : public backend::OpenGLBackend {
public:
    HyprlandOpenGLBackend() = default;
    ~HyprlandOpenGLBackend() override = default;

    // Override to use lazy VAO creation in Hyprland's GL context
    void drawFullscreenQuad() override;
};

} // namespace kaisei::integration::hyprland
