#include "core/registry.h"

#include <cstdlib>
#include <spdlog/spdlog.h>

namespace kaisei::core {

Registry::Registry() : initialized_(false) {
}

void Registry::initialize(const std::filesystem::path& configPath) {
    if (initialized_) {
        spdlog::warn("Registry already initialized");
        return;
    }

    if (configPath.empty()) {
        loadDefaultPaths();
    } else {
        loadConfig(configPath);
    }

    scanAll();
    initialized_ = true;
    spdlog::info("Registry initialized");
}

void Registry::loadDefaultPaths() {
    const char* homeDir = std::getenv("HOME");
    if (!homeDir) {
        spdlog::error("HOME environment variable not set");
        return;
    }

    std::filesystem::path home(homeDir);

    // System-wide installation (will be used when packaged)
    addModuleSearchPath("/usr/share/kaisei/modules");
    addPresetSearchPath("/usr/share/kaisei/presets");

    // User config directory (takes precedence over system)
    addModuleSearchPath(home / ".config/kaisei/modules");
    addPresetSearchPath(home / ".config/kaisei/presets");

    moduleLoader_.setStandardSavePath(home / ".config/kaisei/modules");
    setPresetSavePath(home / ".config/kaisei/presets");

    spdlog::debug("Loaded default search paths");
}

void Registry::loadConfig(const std::filesystem::path& configPath) {
    loadDefaultPaths();
    spdlog::debug("Config loading not yet implemented, using defaults");
}

void Registry::addModuleSearchPath(const std::filesystem::path& path) {
    moduleLoader_.addSearchPath(path);
}

void Registry::addPresetSearchPath(const std::filesystem::path& path) {
    presetLoader_.addSearchPath(path);
}

void Registry::setPresetSavePath(const std::filesystem::path& path) {
    presetLoader_.setStandardSavePath(path);
}

void Registry::scanAll() {
    moduleLoader_.scanModules();
    presetLoader_.scanPresets();

    auto presetNames = presetLoader_.listPresets();
    spdlog::debug("Found {} presets: {}", presetNames.size(),
                 presetNames.empty() ? "none" : [&]() {
                     std::string names;
                     for (size_t i = 0; i < presetNames.size(); ++i) {
                         if (i > 0) names += ", ";
                         names += presetNames[i];
                     }
                     return names;
                 }());
}

void Registry::reload() {
    spdlog::info("Reloading registry...");
    moduleLoader_.reload();
    presetLoader_.reload();
}

ModuleLoader& Registry::modules() {
    return moduleLoader_;
}

PresetLoader& Registry::presets() {
    return presetLoader_;
}

const ModuleLoader& Registry::modules() const {
    return moduleLoader_;
}

const PresetLoader& Registry::presets() const {
    return presetLoader_;
}

bool Registry::validatePreset(const Preset& preset) const {
    for (const auto& module : preset.modules()) {
        if (!moduleLoader_.hasModule(module.moduleName)) {
            spdlog::error("Preset '{}' references unknown module '{}'",
                         preset.name(), module.moduleName);
            return false;
        }
    }
    return true;
}

} // namespace kaisei::core
