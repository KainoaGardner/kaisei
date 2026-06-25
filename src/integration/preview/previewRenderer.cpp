#include "integration/preview/previewRenderer.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "backend/openGLBackend.h"

namespace kaisei::integration::preview {

PreviewRenderer::PreviewRenderer(core::Registry& registry)
    : registry_(registry), sourceTexture_(0), imageWidth_(0), imageHeight_(0),
      mouseLeftButton_(false), mouseRightButton_(false) {
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

    window_->setKeyCallback([this](int key, int action) {
        handleKeyInput(key, action);
    });

    window_->setMousePosCallback([this](double x, double y) {
        handleMousePos(x, y);
    });

    window_->setMouseButtonCallback([this](int button, int action) {
        handleMouseButton(button, action);
    });

    while (!window_->shouldClose()) {
        window_->pollEvents();

        int fbWidth, fbHeight;
        window_->getFramebufferSize(fbWidth, fbHeight);

        backend_->unbindFramebuffer();
        backend_->setViewport(0, 0, fbWidth, fbHeight);
        backend_->clear(0.2f, 0.2f, 0.2f, 1.0f);

        renderer_->render(sourceTexture_, imageWidth_, imageHeight_);

        window_->swapBuffers();
    }

    spdlog::info("Preview window closed");
}

void PreviewRenderer::handleKeyInput(int key, int action) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window_->getHandle(), GLFW_TRUE);
                break;
            case GLFW_KEY_R:
                spdlog::info("Reloading preset...");
                renderer_->reload();
                break;
        }
    }
}

void PreviewRenderer::handleMousePos(double x, double y) {
    // Normalize mouse position to 0-1 range
    int fbWidth, fbHeight;
    window_->getFramebufferSize(fbWidth, fbHeight);

    float normalizedX = static_cast<float>(x) / static_cast<float>(fbWidth);
    float normalizedY = static_cast<float>(y) / static_cast<float>(fbHeight);

    renderer_->setMousePosition(normalizedX, normalizedY);
}

void PreviewRenderer::handleMouseButton(int button, int action) {
    bool pressed = (action == GLFW_PRESS);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mouseLeftButton_ = pressed;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        mouseRightButton_ = pressed;
    }

    renderer_->setMouseButtons(mouseLeftButton_, mouseRightButton_);
}

} // namespace kaisei::integration::preview
