#pragma once

#include <memory>
#include <string>

#include "backend/backend.h"
#include "renderer/multiPassRenderer.h"
#include "integration/preview/previewWindow.h"
#include "core/registry.h"

namespace kaisei::integration::preview {

class PreviewRenderer {
public:
    PreviewRenderer(core::Registry& registry);
    ~PreviewRenderer();

    void run(const std::string& imagePath, const std::string& presetName);

private:
    void handleKeyInput(int key, int action);

    core::Registry& registry_;
    std::unique_ptr<backend::Backend> backend_;
    std::unique_ptr<renderer::MultiPassRenderer> renderer_;
    std::unique_ptr<PreviewWindow> window_;

    uint32_t sourceTexture_;
    int imageWidth_;
    int imageHeight_;
};

} // namespace kaisei::integration::preview
