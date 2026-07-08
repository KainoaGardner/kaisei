#pragma once

#include <memory>
#include <string>

#include "backend/backend.h"
#include "renderer/multiPassRenderer.h"
#include "core/registry.h"

namespace kaisei::integration::hyprland {

class HyprlandRenderer {
public:
    explicit HyprlandRenderer(core::Registry& registry);
    ~HyprlandRenderer();

    void loadPreset(const std::string& name);
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    std::string getCurrentPreset() const { return currentPreset_; }
    std::string getStatus() const;

    void render(uint32_t inputTexture, uint32_t outputFbo, uint32_t width, uint32_t height);

    void checkPresetFileChange();
    void loadCurrentPreset();

private:
    core::Registry& registry_;
    std::unique_ptr<backend::Backend> backend_;
    std::unique_ptr<renderer::MultiPassRenderer> renderer_;

    std::string currentPreset_;
    bool enabled_;

    int inotifyFd_;
    int watchFd_;
    std::filesystem::path currentPresetFile_;
};

} // namespace kaisei::integration::hyprland
