#include "integration/hyprland/hyprlandRenderer.h"

#include <cstdlib>
#include <fstream>
#include <sys/inotify.h>
#include <unistd.h>
#include <GLES3/gl32.h>
#include <spdlog/spdlog.h>
#include "integration/hyprland/hyprlandBackend.h"
#include "utils/format.h"

namespace kaisei::integration::hyprland {

HyprlandRenderer::HyprlandRenderer(core::Registry& registry)
    : registry_(registry), enabled_(false), inotifyFd_(-1), watchFd_(-1) {
    backend_ = std::make_unique<HyprlandOpenGLBackend>();
    if (!backend_->initialize()) {
        throw std::runtime_error("Failed to initialize Hyprland backend");
    }

    // Set up inotify watching for current preset file
    const char* homeDir = std::getenv("HOME");
    if (homeDir) {
        currentPresetFile_ = std::filesystem::path(homeDir) / ".local/share/kaisei/current_preset";

        inotifyFd_ = inotify_init1(IN_NONBLOCK);
        if (inotifyFd_ >= 0) {
            // Watch for modify and create events
            watchFd_ = inotify_add_watch(inotifyFd_, currentPresetFile_.c_str(), IN_MODIFY | IN_CREATE);
            if (watchFd_ < 0) {
                spdlog::debug("Could not watch current preset file (will be created on first preset change)");
            } else {
                spdlog::debug("Watching current preset file: {}", currentPresetFile_.string());
            }
        }
    }
}

HyprlandRenderer::~HyprlandRenderer() {
    spdlog::debug("HyprlandRenderer destructor called");

    if (watchFd_ >= 0) {
        inotify_rm_watch(inotifyFd_, watchFd_);
    }
    if (inotifyFd_ >= 0) {
        close(inotifyFd_);
    }

    spdlog::debug("HyprlandRenderer destroyed");
}

void HyprlandRenderer::loadPreset(const std::string& name) {
    registry_.reload();

    if (!registry_.presets().hasPreset(name)) {
        throw std::runtime_error("Preset '" + name + "' not found");
    }

    const auto* preset = registry_.presets().getPreset(name);

    // Validate that all modules exist before loading
    if (!registry_.validatePreset(*preset)) {
        throw std::runtime_error("Preset '" + name + "' references missing modules");
    }

    if (!renderer_) {
        renderer_ = std::make_unique<renderer::MultiPassRenderer>(*backend_, registry_.modules());
    }

    renderer_->loadPreset(*preset);
    currentPreset_ = name;
    registry_.setCurrentPreset(name);

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
    checkPresetFileChange();

    if (!enabled_ || !renderer_) {
        return;
    }

    renderer_->render(inputTexture, width, height, outputFbo);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        spdlog::error("HyprlandRenderer::render: GL error after render: 0x{:x}", err);
    }
}

void HyprlandRenderer::checkPresetFileChange() {
    if (inotifyFd_ < 0) {
        return;
    }

    char buffer[1024];
    ssize_t len = read(inotifyFd_, buffer, sizeof(buffer));

    if (len <= 0) {
        return;
    }

    // File was modified, read the new preset name
    std::ifstream file(currentPresetFile_);
    if (!file) {
        return;
    }

    std::string newPreset;
    std::getline(file, newPreset);
    file.close();

    if (newPreset.empty() || newPreset == currentPreset_) {
        return;
    }

    // Load the new preset
    try {
        loadPreset(newPreset);
        spdlog::info("Auto-loaded preset '{}' from file change", newPreset);
    } catch (const std::exception& e) {
        spdlog::error("Failed to auto-load preset '{}': {}", newPreset, e.what());
    }
}

void HyprlandRenderer::loadCurrentPreset() {
    if (currentPresetFile_.empty() || !std::filesystem::exists(currentPresetFile_)) {
        throw std::runtime_error("No preset selected. Use 'kaisei preset select <name>' first");
    }

    std::ifstream file(currentPresetFile_);
    if (!file) {
        throw std::runtime_error("Failed to read current preset file");
    }

    std::string presetName;
    std::getline(file, presetName);
    file.close();

    if (presetName.empty()) {
        throw std::runtime_error("No preset selected. Use 'kaisei preset select <name>' first");
    }

    if (presetName != currentPreset_) {
        loadPreset(presetName);
    }
}

} // namespace kaisei::integration::hyprland
