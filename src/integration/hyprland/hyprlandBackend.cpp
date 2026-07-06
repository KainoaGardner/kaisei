#include "integration/hyprland/hyprlandBackend.h"

#include <GLES3/gl32.h>
#include <spdlog/spdlog.h>

#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/gl/GLTexture.hpp>

namespace kaisei::integration::hyprland {

HyprlandOpenGLBackend::HyprlandOpenGLBackend()
    : boundTexture_(0), viewportWidth_(0), viewportHeight_(0) {
}

void HyprlandOpenGLBackend::bindTexture(uint32_t texture, uint32_t slot) {
    OpenGLBackend::bindTexture(texture, slot);

    if (slot == 0) {
        boundTexture_ = texture;
    }
}

void HyprlandOpenGLBackend::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    OpenGLBackend::setViewport(x, y, width, height);

    viewportWidth_ = width;
    viewportHeight_ = height;
}

void HyprlandOpenGLBackend::drawFullscreenQuad() {
    if (boundTexture_ == 0) {
        spdlog::error("drawFullscreenQuad called without binding a texture to slot 0");
        return;
    }

    if (viewportWidth_ == 0 || viewportHeight_ == 0) {
        spdlog::error("drawFullscreenQuad called without setting viewport");
        return;
    }

    uint32_t texWidth, texHeight;
    getTextureSize(boundTexture_, texWidth, texHeight);

    if (texWidth == 0 || texHeight == 0) {
        texWidth = viewportWidth_;
        texHeight = viewportHeight_;
    }

    auto tex = makeShared<Render::GL::CGLTexture>(false);
    tex->m_texID = boundTexture_;
    tex->m_size = {static_cast<double>(texWidth), static_cast<double>(texHeight)};

    CBox box = {0, 0, static_cast<double>(viewportWidth_), static_cast<double>(viewportHeight_)};

    Render::GL::CHyprOpenGLImpl::STextureRenderData renderData;
    renderData.a = 1.0f;

    Render::GL::g_pHyprOpenGL->renderTexture(tex, box, renderData);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        spdlog::error("g_pHyprOpenGL->renderTexture error: 0x{:x}", err);
    }
}

} // namespace kaisei::integration::hyprland
