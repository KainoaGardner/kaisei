#include "integration/hyprland/hyprlandBackend.h"

namespace kaisei::integration::hyprland {

void HyprlandOpenGLBackend::drawFullscreenQuad() {
    // Use parent's lazy VAO creation - now that we're hooking CRenderPass::render(),
    // we're in valid GL context (between begin() and end()), so lazy VAO creation should work
    OpenGLBackend::drawFullscreenQuad();
}

} // namespace kaisei::integration::hyprland
