#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/preset.h"

namespace kaisei::core {

class PresetLoader {
public:
    explicit PresetLoader(const std::vector<std::filesystem::path>& searchPaths = {});

    void addSearchPath(const std::filesystem::path& path);
    void setStandardSavePath(const std::filesystem::path& path);
    void scanPresets();

    Preset* createPreset(const std::string& name,
                        const std::string& version = "1.0",
                        const std::string& description = "");

    void save(const std::string& name);
    void deletePreset(const std::string& name);
    void exportPreset(const std::string& name, const std::filesystem::path& filePath);
    void importPreset(const std::filesystem::path& filePath);

    const Preset* getPreset(const std::string& name) const;
    Preset* getPresetMutable(const std::string& name);
    bool hasPreset(const std::string& name) const;
    void reloadPreset(const std::string& name);

    std::vector<std::string> listPresets() const;

    void reload();
    void clear();

private:
    std::unique_ptr<Preset> loadPreset(const std::filesystem::path& filePath) const;
    void savePreset(const Preset& preset, const std::filesystem::path& filePath) const;
    void loadPresetsFromPath(const std::filesystem::path& path);
    std::filesystem::path getPresetFilePath(const std::string& name) const;

    std::vector<std::filesystem::path> searchPaths_;
    std::filesystem::path standardSavePath_;
    std::map<std::string, std::unique_ptr<Preset>> presets_;
};

} // namespace kaisei::core
