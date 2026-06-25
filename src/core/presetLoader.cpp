#include "core/presetLoader.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <sstream>

#include <toml++/toml.h>
#include <spdlog/spdlog.h>

namespace kaisei::core {

namespace {

std::string tomlValueToString(const toml::node& value) {
    if (auto strValue = value.value<std::string>()) {
        return *strValue;
    }

    if (auto intValue = value.value<int64_t>()) {
        return std::to_string(*intValue);
    }

    if (auto floatValue = value.value<double>()) {
        return std::to_string(*floatValue);
    }

    if (auto arrayValue = value.as_array()) {
        std::ostringstream oss;
        for (size_t i = 0; i < arrayValue->size(); ++i) {
            if (i > 0) oss << ",";

            if (auto elemFloat = (*arrayValue)[i].value<double>()) {
                oss << *elemFloat;
            } else if (auto elemInt = (*arrayValue)[i].value<int64_t>()) {
                oss << *elemInt;
            } else if (auto elemStr = (*arrayValue)[i].value<std::string>()) {
                oss << *elemStr;
            }
        }
        return oss.str();
    }

    return "";
}

}  // namespace

PresetLoader::PresetLoader(const std::vector<std::filesystem::path>& searchPaths)
    : searchPaths_(searchPaths) {
    if (!searchPaths_.empty()) {
        scanPresets();
    }
}

void PresetLoader::addSearchPath(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        spdlog::warn("Search path does not exist: {}", path.string());
        return;
    }

    searchPaths_.push_back(path);
    spdlog::debug("Added preset search path: {}", path.string());
}

void PresetLoader::setStandardSavePath(const std::filesystem::path& path) {
    standardSavePath_ = path;

    if (!std::filesystem::exists(standardSavePath_)) {
        std::filesystem::create_directories(standardSavePath_);
        spdlog::info("Created preset save directory: {}", standardSavePath_.string());
    }

    spdlog::debug("Set standard preset save path: {}", path.string());
}

void PresetLoader::scanPresets() {
    clear();

    for (const auto& path : searchPaths_) {
        loadPresetsFromPath(path);
    }

    spdlog::info("Loaded {} presets", presets_.size());
}

void PresetLoader::loadPresetsFromPath(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        spdlog::warn("Invalid preset path: {}", path.string());
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (!entry.is_regular_file()) continue;

        const auto& filePath = entry.path();
        if (filePath.extension() != ".preset") continue;

        try {
            auto preset = loadPreset(filePath);
            const auto& name = preset->name();

            if (presets_.count(name)) {
                spdlog::warn("Duplicate preset '{}', skipping {}", name, filePath.string());
                continue;
            }

            presets_[name] = std::move(preset);
            spdlog::debug("Loaded preset: {} from {}", name, filePath.string());

        } catch (const std::exception& e) {
            spdlog::error("Failed to load preset from {}: {}", filePath.string(), e.what());
        }
    }
}

std::unique_ptr<Preset> PresetLoader::loadPreset(const std::filesystem::path& filePath) const {
    if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("Preset file not found: " + filePath.string());
    }

    try {
        auto config = toml::parse_file(filePath.string());

        auto presetTable = config["preset"];
        std::string name = presetTable["name"].value_or<std::string>("");
        std::string version = presetTable["version"].value_or<std::string>("1.0");
        std::string description = presetTable["description"].value_or<std::string>("");

        if (name.empty()) {
            throw std::runtime_error("Preset name is required");
        }

        auto preset = std::make_unique<Preset>(name, version);
        preset->setDescription(description);
        preset->setSourcePath(filePath);

        if (auto modulesArray = config["modules"].as_array()) {
            for (auto& moduleNode : *modulesArray) {
                auto moduleTable = moduleNode.as_table();
                if (!moduleTable) continue;

                std::string moduleName = (*moduleTable)["name"].value_or<std::string>("");
                if (moduleName.empty()) {
                    spdlog::warn("Skipping module with no name in preset {}", name);
                    continue;
                }

                std::map<std::string, std::string> overrides;
                if (auto overridesTable = (*moduleTable)["overrides"].as_table()) {
                    for (auto& [key, value] : *overridesTable) {
                        std::string overrideValue = tomlValueToString(value);
                        if (!overrideValue.empty()) {
                            overrides[std::string(key.str())] = overrideValue;
                        }
                    }
                }

                preset->addModule(moduleName, overrides);
            }
        }

        spdlog::info("Loaded preset '{}' with {} modules", name, preset->modules().size());
        return preset;

    } catch (const toml::parse_error& err) {
        throw std::runtime_error("Failed to parse preset TOML: " + std::string(err.what()));
    }
}

void PresetLoader::savePreset(const Preset& preset, const std::filesystem::path& filePath) const {
    toml::table root;

    root.insert("preset", toml::table{
        {"name", preset.name()},
        {"version", preset.version()},
        {"description", preset.description()}
    });

    toml::array modulesArray;
    for (const auto& module : preset.modules()) {
        toml::table moduleTable{
            {"name", module.moduleName}
        };

        if (!module.uniformOverrides.empty()) {
            toml::table overridesTable;
            for (const auto& [key, value] : module.uniformOverrides) {
                overridesTable.insert(key, value);
            }
            moduleTable.insert("overrides", overridesTable);
        }

        modulesArray.push_back(moduleTable);
    }

    root.insert("modules", modulesArray);

    std::ofstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filePath.string());
    }

    file << root;
    spdlog::info("Saved preset '{}' to {}", preset.name(), filePath.string());
}

std::filesystem::path PresetLoader::getPresetFilePath(const std::string& name) const {
    if (standardSavePath_.empty()) {
        throw std::runtime_error("Standard save path not set");
    }
    return standardSavePath_ / (name + ".preset");
}

Preset* PresetLoader::createPreset(const std::string& name,
                                   const std::string& version,
                                   const std::string& description) {
    if (presets_.count(name)) {
        throw std::runtime_error("Preset already exists: " + name);
    }

    auto preset = std::make_unique<Preset>(name, version);
    preset->setDescription(description);

    auto filePath = getPresetFilePath(name);
    preset->setSourcePath(filePath);
    savePreset(*preset, filePath);

    auto* ptr = preset.get();
    presets_[name] = std::move(preset);

    spdlog::info("Created preset '{}' at {}", name, filePath.string());
    return ptr;
}

void PresetLoader::save(const std::string& name) {
    auto it = presets_.find(name);
    if (it == presets_.end()) {
        throw std::runtime_error("Preset not found: " + name);
    }

    auto filePath = it->second->sourcePath().empty()
                    ? getPresetFilePath(name)
                    : it->second->sourcePath();
    savePreset(*it->second, filePath);
}

void PresetLoader::deletePreset(const std::string& name) {
    auto it = presets_.find(name);
    if (it == presets_.end()) {
        throw std::runtime_error("Preset not found: " + name);
    }

    auto filePath = it->second->sourcePath().empty()
                    ? getPresetFilePath(name)
                    : it->second->sourcePath();

    if (std::filesystem::exists(filePath)) {
        std::filesystem::remove(filePath);
        spdlog::info("Deleted preset file: {}", filePath.string());
    }

    presets_.erase(it);
    spdlog::info("Deleted preset '{}'", name);
}

void PresetLoader::exportPreset(const std::string& name, const std::filesystem::path& filePath) {
    auto it = presets_.find(name);
    if (it == presets_.end()) {
        throw std::runtime_error("Preset not found: " + name);
    }

    savePreset(*it->second, filePath);
    spdlog::info("Exported preset '{}' to {}", name, filePath.string());
}

void PresetLoader::importPreset(const std::filesystem::path& filePath) {
    auto preset = loadPreset(filePath);
    const auto& name = preset->name();

    if (presets_.count(name)) {
        spdlog::warn("Preset '{}' already exists, replacing", name);
    }

    auto savePath = getPresetFilePath(name);
    savePreset(*preset, savePath);

    presets_[name] = std::move(preset);
    spdlog::info("Imported preset '{}' to {}", name, savePath.string());
}

const Preset* PresetLoader::getPreset(const std::string& name) const {
    auto it = presets_.find(name);
    if (it == presets_.end()) {
        throw std::runtime_error("Preset not found: " + name);
    }
    return it->second.get();
}

Preset* PresetLoader::getPresetMutable(const std::string& name) {
    auto it = presets_.find(name);
    if (it == presets_.end()) {
        throw std::runtime_error("Preset not found: " + name);
    }
    return it->second.get();
}

bool PresetLoader::hasPreset(const std::string& name) const {
    return presets_.count(name) > 0;
}

std::vector<std::string> PresetLoader::listPresets() const {
    std::vector<std::string> names;
    names.reserve(presets_.size());

    for (const auto& [name, preset] : presets_) {
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    return names;
}

void PresetLoader::reload() {
    spdlog::info("Reloading presets...");
    scanPresets();
}

void PresetLoader::clear() {
    presets_.clear();
}

} // namespace kaisei::core
