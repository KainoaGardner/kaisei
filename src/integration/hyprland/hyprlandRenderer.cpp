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

struct TextureSlotState {
    GLint tex2D = 0;
    GLint texExternal = 0;
    GLint texRect = 0;
};
 
HyprlandRenderer::HyprlandRenderer(core::Registry& registry)
    : registry_(registry), enabled_(false), inotifyFd_(-1), watchFd_(-1) {
    backend_ = std::make_unique<HyprlandOpenGLBackend>();
    if (!backend_->initialize()) {
        throw std::runtime_error("Failed to initialize Hyprland backend");
    }

    const char* homeDir = std::getenv("HOME");
    if (homeDir) {
        currentPresetFile_ = std::filesystem::path(homeDir) / ".local/share/kaisei/current_preset";

        inotifyFd_ = inotify_init1(IN_NONBLOCK);
        if (inotifyFd_ >= 0) {
            auto parentDir = currentPresetFile_.parent_path();
            std::filesystem::create_directories(parentDir);
            watchFd_ = inotify_add_watch(inotifyFd_, parentDir.c_str(), IN_MODIFY | IN_CREATE | IN_CLOSE_WRITE);
            if (watchFd_ < 0) {
                spdlog::error("Could not watch current preset directory: {}", parentDir.string());
            } else {
                spdlog::debug("Watching current preset directory: {}", parentDir.string());
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
    const_cast<HyprlandRenderer*>(this)->checkPresetFileChange();
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

    #ifndef GL_TEXTURE_EXTERNAL_OES
    #define GL_TEXTURE_EXTERNAL_OES 0x8D65
    #endif
    #ifndef GL_TEXTURE_BINDING_EXTERNAL_OES
    #define GL_TEXTURE_BINDING_EXTERNAL_OES 0x8D7F
    #endif
    #ifndef GL_TEXTURE_RECTANGLE
    #define GL_TEXTURE_RECTANGLE 0x84F5
    #endif
    #ifndef GL_TEXTURE_BINDING_RECTANGLE
    #define GL_TEXTURE_BINDING_RECTANGLE 0x84F6
    #endif

    GLint originalActiveTexture = GL_TEXTURE0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &originalActiveTexture);


    TextureSlotState savedSlots[5];

    for (int slot = 0; slot < 5; ++slot) {
        glActiveTexture(GL_TEXTURE0 + slot);
        
        // 2D target
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedSlots[slot].tex2D);
        while (glGetError() != GL_NO_ERROR) {}

        // External OES target
        glGetIntegerv(GL_TEXTURE_BINDING_EXTERNAL_OES, &savedSlots[slot].texExternal);
        if (glGetError() != GL_NO_ERROR) {
            savedSlots[slot].texExternal = 0;
        }

        // Rectangle target
        glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE, &savedSlots[slot].texRect);
        if (glGetError() != GL_NO_ERROR) {
            savedSlots[slot].texRect = 0;
        }
    }

    glActiveTexture(originalActiveTexture);

    try {
        renderer_->render(inputTexture, width, height, outputFbo);
    } catch (const std::exception& e) {
        spdlog::error("Shader rendering or compilation failed: {}", e.what());
        spdlog::warn("Disabling Kaisei effects to prevent crashing Hyprland");
        enabled_ = false;
    }

    for (int slot = 4; slot >= 0; --slot) {
        glActiveTexture(GL_TEXTURE0 + slot);

        glBindTexture(GL_TEXTURE_2D, savedSlots[slot].tex2D);
        while (glGetError() != GL_NO_ERROR) {}

        glBindTexture(GL_TEXTURE_EXTERNAL_OES, savedSlots[slot].texExternal);
        while (glGetError() != GL_NO_ERROR) {}

        glBindTexture(GL_TEXTURE_RECTANGLE, savedSlots[slot].texRect);
        while (glGetError() != GL_NO_ERROR) {}
    }

    glActiveTexture(originalActiveTexture);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        spdlog::error("HyprlandRenderer::render: GL error after render: 0x{:x}", err);
    }
}

void HyprlandRenderer::checkPresetFileChange() {
    if (inotifyFd_ < 0 || watchFd_ < 0) {
        return;
    }

    char buffer[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
    ssize_t len = read(inotifyFd_, buffer, sizeof(buffer));

    if (len <= 0) {
        return;
    }

    bool fileChanged = false;
    const struct inotify_event* event;
    for (char* ptr = buffer; ptr < buffer + len; ptr += sizeof(struct inotify_event) + event->len) {
        event = reinterpret_cast<const struct inotify_event*>(ptr);
        if (event->len > 0) {
            std::string filename(event->name);
            if (filename == "current_preset") {
                fileChanged = true;
                break;
            }
        }
    }

    if (!fileChanged) {
        return;
    }

    std::ifstream file(currentPresetFile_);
    if (!file) {
        return;
    }

    std::string newPreset;
    std::getline(file, newPreset);
    file.close();

    if (!newPreset.empty() && newPreset.back() == '\r') {
        newPreset.pop_back();
    }

    if (newPreset.empty() || newPreset == currentPreset_) {
        return;
    }

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

    loadPreset(presetName);
}

} // namespace kaisei::integration::hyprland
