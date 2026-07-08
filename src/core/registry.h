#pragma once

#include <filesystem>

#include "core/moduleLoader.h"
#include "core/presetLoader.h"

namespace kaisei::core {

class Registry {
public:
    Registry();

    void initialize(const std::filesystem::path& configPath = "");
    void addModuleSearchPath(const std::filesystem::path& path);
    void addPresetSearchPath(const std::filesystem::path& path);
    void setPresetSavePath(const std::filesystem::path& path);

    void scanAll();
    void reload();

    ModuleLoader& modules();
    PresetLoader& presets();

    const ModuleLoader& modules() const;
    const PresetLoader& presets() const;

    bool validatePreset(const Preset& preset) const;

    void setCurrentPreset(const std::string& name);
    void clearCurrentPreset();
    std::string getCurrentPreset() const;
    std::string nextPreset();
    std::string prevPreset();

private:
    void loadDefaultPaths();
    void loadConfig(const std::filesystem::path& configPath);
    void updatePresetList();
    void writeCurrentPresetFile();
    void readCurrentPresetFile();

    ModuleLoader moduleLoader_;
    PresetLoader presetLoader_;
    bool initialized_;

    std::string currentPreset_;
    std::vector<std::string> availablePresets_;
    int currentPresetIndex_ = -1;
    std::filesystem::path currentPresetFile_;
};

} // namespace kaisei::core
