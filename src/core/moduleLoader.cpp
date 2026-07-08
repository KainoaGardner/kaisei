#include "core/moduleLoader.h"

#include <algorithm>
#include <fstream>
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

void ModuleLoader::setStandardSavePath(const std::filesystem::path& path) {
    standardSavePath_ = path;

    if (!std::filesystem::exists(standardSavePath_)) {
        std::filesystem::create_directories(standardSavePath_);
        spdlog::info("Created module save directory: {}", standardSavePath_.string());
    }

    spdlog::debug("Set standard module save path: {}", path.string());
}

std::filesystem::path ModuleLoader::getModuleBasePath(const std::string& name) const {
    if (standardSavePath_.empty()) {
        throw std::runtime_error("Standard save path not set");
    }
    return standardSavePath_ / name;
}

void ModuleLoader::createModule(const std::string& name,
                                const std::string& version,
                                const std::string& description,
                                bool stage) {
    if (modules_.count(name)) {
        throw std::runtime_error("Module already exists: " + name);
    }

    if (!std::filesystem::exists(standardSavePath_)) {
        std::filesystem::create_directories(standardSavePath_);
    }

    std::filesystem::path moduleBasePath = standardSavePath_ / name;
    if (!std::filesystem::exists(moduleBasePath)) {
        std::filesystem::create_directories(moduleBasePath);
    }

    std::filesystem::path tomlPath = moduleBasePath / (name + ".toml");
    std::filesystem::path fragPath = moduleBasePath / (name + ".frag");

    std::ofstream tomlFile(tomlPath);
    if (!tomlFile.is_open()) {
        throw std::runtime_error("Failed to create module file: " + tomlPath.string());
    }

    tomlFile << "[module]\n";
    tomlFile << "name = \"" << name << "\"\n";
    tomlFile << "version = \"" << version << "\"\n";
    tomlFile << "author = \"\"\n";
    tomlFile << "description = \"" << description << "\"\n";
    tomlFile << "tags = []\n";
    tomlFile << "type = \"" << (stage ? "stage" : "module") << "\"  # Type: \"module\" for single-pass effects, \"stage\" for multi-pass\n";
    tomlFile << "\n";

    if (stage) {
        tomlFile << "# Multi-pass shader configuration\n";
        tomlFile << "# Each pass processes the output of the previous pass\n";
        tomlFile << "[[passes]]\n";
        tomlFile << "name = \"pass1\"\n";
        tomlFile << "file = \"" << name << "_pass1.frag\"\n";
        tomlFile << "\n";
        tomlFile << "[[passes]]\n";
        tomlFile << "name = \"pass2\"\n";
        tomlFile << "file = \"" << name << "_pass2.frag\"\n";
        tomlFile << "\n";
    } else {
        tomlFile << "[shader]\n";
        tomlFile << "file = \"" << name << ".frag\"\n";
        tomlFile << "\n";
    }

    tomlFile << "# Add custom parameters for your shader\n";
    tomlFile << "# [[uniforms]]\n";
    tomlFile << "# name = \"u_intensity\"\n";
    tomlFile << "# type = \"float\"\n";
    tomlFile << "# default = \"1.0\"\n";
    tomlFile << "# description = \"Effect intensity\"\n";
    tomlFile << "# min = 0.0\n";
    tomlFile << "# max = 2.0\n";
    tomlFile.close();

    if (stage) {
        auto fragPath1 = moduleBasePath / (name + "_pass1.frag");
        auto fragPath2 = moduleBasePath / (name + "_pass2.frag");

        std::ofstream fragFile1(fragPath1);
        if (!fragFile1.is_open()) {
            throw std::runtime_error("Failed to create shader file: " + fragPath1.string());
        }

        fragFile1 << "// " << name << " - Pass 1 - " << description << "\n";
        fragFile1 << "// Version: " << version << "\n";
        fragFile1 << "#version 450 core\n";
        fragFile1 << "\n";
        fragFile1 << "in vec2 v_texCoord;\n";
        fragFile1 << "out vec4 fragColor;\n";
        fragFile1 << "\n";
        fragFile1 << "uniform sampler2D u_inputTexture;\n";
        fragFile1 << "\n";
        fragFile1 << "// Additional automatic uniforms available:\n";
        fragFile1 << "// uniform vec2 u_resolution;         // Screen resolution\n";
        fragFile1 << "// uniform vec2 u_texelSize;          // 1.0 / u_resolution\n";
        fragFile1 << "// uniform float u_time;              // Time in seconds\n";
        fragFile1 << "// uniform float u_deltaTime;         // Time since last frame\n";
        fragFile1 << "\n";
        fragFile1 << "void main() {\n";
        fragFile1 << "    vec4 inputColor = texture(u_inputTexture, v_texCoord);\n";
        fragFile1 << "\n";
        fragFile1 << "    // First pass - modify inputColor here\n";
        fragFile1 << "\n";
        fragFile1 << "    fragColor = vec4(inputColor);\n";
        fragFile1 << "}\n";
        fragFile1.close();

        std::ofstream fragFile2(fragPath2);
        if (!fragFile2.is_open()) {
            throw std::runtime_error("Failed to create shader file: " + fragPath2.string());
        }

        fragFile2 << "// " << name << " - Pass 2 - " << description << "\n";
        fragFile2 << "// Version: " << version << "\n";
        fragFile2 << "#version 450 core\n";
        fragFile2 << "\n";
        fragFile2 << "in vec2 v_texCoord;\n";
        fragFile2 << "out vec4 fragColor;\n";
        fragFile2 << "\n";
        fragFile2 << "uniform sampler2D u_inputTexture;\n";
        fragFile2 << "\n";
        fragFile2 << "// Additional automatic uniforms available:\n";
        fragFile2 << "// uniform vec2 u_resolution;         // Screen resolution\n";
        fragFile2 << "// uniform vec2 u_texelSize;          // 1.0 / u_resolution\n";
        fragFile2 << "// uniform float u_time;              // Time in seconds\n";
        fragFile2 << "// uniform float u_deltaTime;         // Time since last frame\n";
        fragFile2 << "\n";
        fragFile2 << "void main() {\n";
        fragFile2 << "    vec4 inputColor = texture(u_inputTexture, v_texCoord);\n";
        fragFile2 << "\n";
        fragFile2 << "    // Second pass - modify inputColor here\n";
        fragFile2 << "\n";
        fragFile2 << "    fragColor = vec4(inputColor);\n";
        fragFile2 << "}\n";
        fragFile2.close();

        spdlog::info("Created multi-pass module '{}' at {}, {}, and {}", name, tomlPath.string(), fragPath1.string(), fragPath2.string());
    } else {
        std::ofstream fragFile(fragPath);
        if (!fragFile.is_open()) {
            throw std::runtime_error("Failed to create shader file: " + fragPath.string());
        }

        fragFile << "// " << name << " - " << description << "\n";
        fragFile << "// Version: " << version << "\n";
        fragFile << "#version 450 core\n";
        fragFile << "\n";
        fragFile << "in vec2 v_texCoord;\n";
        fragFile << "out vec4 fragColor;\n";
        fragFile << "\n";
        fragFile << "uniform sampler2D u_inputTexture;\n";
        fragFile << "\n";
        fragFile << "// Additional automatic uniforms available:\n";
        fragFile << "// uniform vec2 u_resolution;         // Screen resolution\n";
        fragFile << "// uniform vec2 u_texelSize;          // 1.0 / u_resolution\n";
        fragFile << "// uniform float u_time;              // Time in seconds\n";
        fragFile << "// uniform float u_deltaTime;         // Time since last frame\n";
        fragFile << "\n";
        fragFile << "void main() {\n";
        fragFile << "    vec4 inputColor = texture(u_inputTexture, v_texCoord);\n";
        fragFile << "\n";
        fragFile << "    // Passthrough - modify inputColor here to create your effect\n";
        fragFile << "\n";
        fragFile << "    fragColor = vec4(inputColor);\n";
        fragFile << "}\n";
        fragFile.close();

        spdlog::info("Created module '{}' at {} and {}", name, tomlPath.string(), fragPath.string());
    }

    spdlog::info("Edit the files to customize your module, then reload with 'kaisei module reload'");
}

void ModuleLoader::deleteModule(const std::string& name) {
    std::filesystem::path moduleDir;
    std::filesystem::path tomlPath;
    bool isStageModule = false;
    bool foundInMemory = false;

    auto it = modules_.find(name);
    if (it != modules_.end()) {
        auto modulePath = it->second->path(); 

        if (modulePath.filename() == name) {
            moduleDir = modulePath;
            isStageModule = true;
            spdlog::debug("Found module '{}' in directory: {}", name, moduleDir.string());
        } else {
            tomlPath = modulePath / (name + ".toml");
            spdlog::debug("Found module '{}' in memory at {}", name, tomlPath.string());
        }
        foundInMemory = true;
    } else {
        spdlog::debug("Module '{}' not in memory, searching {} paths on disk", name, searchPaths_.size());
        bool found = false;
        for (const auto& searchPath : searchPaths_) {
            spdlog::debug("  Checking search path: {}", searchPath.string());
            if (!std::filesystem::exists(searchPath)) {
                spdlog::debug("    Path does not exist");
                continue;
            }

            auto potentialDir = searchPath / name;
            auto potentialDirToml = potentialDir / (name + ".toml");
            if (std::filesystem::exists(potentialDirToml)) {
                moduleDir = potentialDir;
                isStageModule = true;
                found = true;
                spdlog::debug("    Found module directory: {}", moduleDir.string());
                break;
            }

            auto potentialToml = searchPath / (name + ".toml");
            if (std::filesystem::exists(potentialToml)) {
                tomlPath = potentialToml;
                found = true;
                spdlog::debug("    Found module file: {}", tomlPath.string());
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("Module '" + name + "' not found in memory or on disk");
        }
    }

    bool deletedAny = false;

    if (isStageModule && std::filesystem::exists(moduleDir)) {
        std::filesystem::remove_all(moduleDir);
        spdlog::info("Deleted module directory: {}", moduleDir.string());
        deletedAny = true;
    } else if (std::filesystem::exists(tomlPath)) {
        auto baseDir = tomlPath.parent_path();
        auto baseName = tomlPath.stem().string();
        auto fragPath = baseDir / (baseName + ".frag");

        std::filesystem::remove(tomlPath);
        spdlog::info("Deleted module file: {}", tomlPath.string());
        deletedAny = true;

        if (std::filesystem::exists(fragPath)) {
            std::filesystem::remove(fragPath);
            spdlog::info("Deleted shader file: {}", fragPath.string());
        }
    }

    if (foundInMemory) {
        modules_.erase(it);
    }

    if (deletedAny) {
        spdlog::info("Deleted module '{}'", name);
    } else {
        spdlog::warn("No files found to delete for module '{}'", name);
    }
}

void ModuleLoader::reload() {
    spdlog::info("Reloading modules...");
    scanModules();
}

void ModuleLoader::clear() {
    modules_.clear();
}

} // namespace kaisei::core
