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

    addModuleSearchPath("/usr/share/kaisei/modules");
    addModuleSearchPath(home / ".local/share/kaisei/modules");

    addPresetSearchPath("/usr/share/kaisei/presets");
    addPresetSearchPath(home / ".config/kaisei/presets");

    setPresetSavePath(home / ".config/kaisei/presets");

    spdlog::debug("Loaded default search paths");
}

void Registry::loadConfig(const std::filesystem::path& configPath) {
    // TODO: Parse config.toml and set paths
    // For now, use default paths
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
