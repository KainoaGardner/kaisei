#include "core/moduleLoader.h"

#include <algorithm>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace kaisei::core {

ModuleLoader::ModuleLoader(const std::vector<std::filesystem::path>& searchPaths)
    : searchPaths_(searchPaths) {
    if (!searchPaths_.empty()) {
        scanModules();
    }
}

void ModuleLoader::addSearchPath(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        spdlog::warn("Search path does not exist: {}", path.string());
        return;
    }

    searchPaths_.push_back(path);
    spdlog::debug("Added module search path: {}", path.string());
}

void ModuleLoader::scanModules() {
    clear();

    for (const auto& path : searchPaths_) {
        loadModulesFromPath(path);
    }

    spdlog::info("Loaded {} modules", modules_.size());
}

void ModuleLoader::loadModulesFromPath(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        spdlog::warn("Invalid module path: {}", path.string());
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (!entry.is_regular_file()) continue;

        const auto& filePath = entry.path();
        if (filePath.extension() != ".toml") continue;

        try {
            auto module = std::make_unique<Module>(filePath);
            const auto& name = module->metadata().name;

            if (modules_.count(name)) {
                spdlog::warn("Duplicate module '{}', skipping {}", name, filePath.string());
                continue;
            }

            modules_[name] = std::move(module);
            spdlog::debug("Loaded module: {} from {}", name, filePath.string());

        } catch (const std::exception& e) {
            spdlog::error("Failed to load module from {}: {}", filePath.string(), e.what());
        }
    }
}

const Module* ModuleLoader::getModule(const std::string& name) const {
    auto it = modules_.find(name);
    if (it == modules_.end()) {
        throw std::runtime_error("Module not found: " + name);
    }
    return it->second.get();
}

bool ModuleLoader::hasModule(const std::string& name) const {
    return modules_.count(name) > 0;
}

std::vector<std::string> ModuleLoader::listModules() const {
    std::vector<std::string> names;
    names.reserve(modules_.size());

    for (const auto& [name, module] : modules_) {
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    return names;
}

std::vector<const Module*> ModuleLoader::findByTag(const std::string& tag) const {
    std::vector<const Module*> result;

    for (const auto& [name, module] : modules_) {
        const auto& tags = module->metadata().tags;
        if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
            result.push_back(module.get());
        }
    }

    return result;
}

void ModuleLoader::reload() {
    spdlog::info("Reloading modules...");
    scanModules();
}

void ModuleLoader::clear() {
    modules_.clear();
}

} // namespace kaisei::core
