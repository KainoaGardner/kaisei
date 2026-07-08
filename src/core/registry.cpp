#include "core/registry.h"

#include <cstdlib>
#include <fstream>
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
    readCurrentPresetFile();
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
    addPresetSearchPath("/usr/share/kaisei/presets");

    addModuleSearchPath(home / ".config/kaisei/modules");
    addPresetSearchPath(home / ".config/kaisei/presets");

    moduleLoader_.setStandardSavePath(home / ".config/kaisei/modules");
    setPresetSavePath(home / ".config/kaisei/presets");

    currentPresetFile_ = home / ".local/share/kaisei/current_preset";

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
    updatePresetList();
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

void Registry::updatePresetList() {
    availablePresets_ = presetLoader_.listPresets();

    if (!currentPreset_.empty()) {
        auto it = std::find(availablePresets_.begin(), availablePresets_.end(), currentPreset_);
        if (it != availablePresets_.end()) {
            currentPresetIndex_ = std::distance(availablePresets_.begin(), it);
        } else {
            currentPresetIndex_ = -1;
            currentPreset_.clear();
        }
    }
}

void Registry::setCurrentPreset(const std::string& name) {
    currentPreset_ = name;
    updatePresetList();
    writeCurrentPresetFile();
}

void Registry::clearCurrentPreset() {
    currentPreset_.clear();
    currentPresetIndex_ = -1;

    if (!currentPresetFile_.empty() && std::filesystem::exists(currentPresetFile_)) {
        std::filesystem::remove(currentPresetFile_);
    }
}

std::string Registry::getCurrentPreset() const {
    return currentPreset_;
}

std::string Registry::nextPreset() {
    updatePresetList();

    if (availablePresets_.empty()) {
        throw std::runtime_error("No presets available");
    }

    currentPresetIndex_ = (currentPresetIndex_ + 1) % availablePresets_.size();
    currentPreset_ = availablePresets_[currentPresetIndex_];

    spdlog::debug("Cycled to next preset: {} (index {}/{})",
                  currentPreset_, currentPresetIndex_ + 1, availablePresets_.size());

    writeCurrentPresetFile();
    return currentPreset_;
}

std::string Registry::prevPreset() {
    updatePresetList();

    if (availablePresets_.empty()) {
        throw std::runtime_error("No presets available");
    }

    currentPresetIndex_ = (currentPresetIndex_ - 1 + availablePresets_.size()) % availablePresets_.size();
    currentPreset_ = availablePresets_[currentPresetIndex_];

    spdlog::debug("Cycled to previous preset: {} (index {}/{})",
                  currentPreset_, currentPresetIndex_ + 1, availablePresets_.size());

    writeCurrentPresetFile();
    return currentPreset_;
}

void Registry::writeCurrentPresetFile() {
    if (currentPresetFile_.empty()) {
        return;
    }

    auto dir = currentPresetFile_.parent_path();
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    std::ofstream file(currentPresetFile_);
    if (!file) {
        spdlog::warn("Failed to write current preset file: {}", currentPresetFile_.string());
        return;
    }

    file << currentPreset_;
    file.close();
}

void Registry::readCurrentPresetFile() {
    if (currentPresetFile_.empty() || !std::filesystem::exists(currentPresetFile_)) {
        return;
    }

    std::ifstream file(currentPresetFile_);
    if (!file) {
        return;
    }

    std::string presetName;
    std::getline(file, presetName);
    file.close();

    if (!presetName.empty() && presetLoader_.hasPreset(presetName)) {
        currentPreset_ = presetName;
        updatePresetList();
        spdlog::debug("Read current preset from file: {}", presetName);
    }
}

} // namespace kaisei::core
