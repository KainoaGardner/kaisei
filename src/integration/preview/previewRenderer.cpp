#include "integration/preview/previewRenderer.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "backend/openGLBackend.h"

namespace kaisei::integration::preview {

PreviewRenderer::PreviewRenderer(core::Registry& registry)
    : registry_(registry), sourceTexture_(0), imageWidth_(0), imageHeight_(0) {
}

PreviewRenderer::~PreviewRenderer() {
    if (backend_ && sourceTexture_) {
        backend_->deleteTexture(sourceTexture_);
    }
}

void PreviewRenderer::run(const std::string& imagePath, const std::string& presetName) {
    if (!registry_.presets().hasPreset(presetName)) {
        spdlog::error("Preset '{}' not found", presetName);
        return;
    }

    const auto* preset = registry_.presets().getPreset(presetName);

    window_ = std::make_unique<PreviewWindow>(1280, 720, "Kaisei Preview - " + presetName);

    backend_ = std::make_unique<backend::OpenGLBackend>();
    if (!backend_->initialize()) {
        spdlog::error("Failed to initialize backend");
        return;
    }

    sourceTexture_ = backend_->loadTextureFromFile(imagePath);
    backend_->getTextureSize(sourceTexture_,
        *reinterpret_cast<uint32_t*>(&imageWidth_),
        *reinterpret_cast<uint32_t*>(&imageHeight_));

    spdlog::info("Loaded image: {}x{}", imageWidth_, imageHeight_);

    renderer_ = std::make_unique<renderer::MultiPassRenderer>(*backend_, registry_.modules());
    renderer_->loadPreset(*preset);

    while (!window_->shouldClose()) {
        window_->pollEvents();

        int fbWidth, fbHeight;
        window_->getFramebufferSize(fbWidth, fbHeight);

        uint32_t outputTex = renderer_->render(sourceTexture_, imageWidth_, imageHeight_);

        backend_->unbindFramebuffer();
        backend_->setViewport(0, 0, fbWidth, fbHeight);
        backend_->clear(0.2f, 0.2f, 0.2f, 1.0f);

        uint32_t passthroughProg = renderer_->getPassthroughProgram();
        backend_->useProgram(passthroughProg);
        backend_->bindTexture(outputTex, 0);
        backend_->setUniformInt(passthroughProg, "u_inputTexture", 0);
        backend_->drawFullscreenQuad();

        window_->swapBuffers();
    }

    spdlog::info("Preview window closed");
}

} // namespace kaisei::integration::preview
