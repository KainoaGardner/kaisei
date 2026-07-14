#include "core/module.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <toml++/toml.h>
#include <spdlog/spdlog.h>

namespace kaisei::core {

namespace {

EffectType parseEffectType(const std::string& typeStr) {
    if (typeStr == "module") return EffectType::Module;
    if (typeStr == "stage") return EffectType::Stage;
    throw std::runtime_error("Unknown effect type: " + typeStr);
}

}

Module::Module(const std::filesystem::path& moduleFile) : modulePath_(moduleFile.parent_path()) {
    spdlog::debug("Loading module from: {}", moduleFile.string());

    if (!std::filesystem::exists(moduleFile)) {
        throw std::runtime_error("Module file not found: " + moduleFile.string());
    }

    loadMetadata(moduleFile);

    if (metadata_.type == EffectType::Module) {
        auto shaderFile = modulePath_ / metadata_.shaderFile;
        loadShader(shaderFile);
    } else if (metadata_.type == EffectType::Stage) {
        loadPasses(modulePath_);
    }

    spdlog::info("Loaded module: {} ({})", metadata_.name, metadata_.description);
}

void Module::loadMetadata(const std::filesystem::path& tomlFile) {
    try {
        auto config = toml::parse_file(tomlFile.string());

        auto module = config["module"];
        metadata_.name = module["name"].value_or<std::string>("");
        metadata_.version = module["version"].value_or<std::string>("1.0.0");
        metadata_.author = module["author"].value_or<std::string>("unknown");
        metadata_.description = module["description"].value_or<std::string>("");

        if (auto tags = module["tags"].as_array()) {
            for (auto& tag : *tags) {
                metadata_.tags.push_back(tag.value_or<std::string>(""));
            }
        }

        std::string typeStr = module["type"].value_or<std::string>("module");
        metadata_.type = parseEffectType(typeStr);

        auto shader = config["shader"];
        metadata_.shaderFile = shader["file"].value_or<std::string>("");

        if (auto uniformsArray = config["uniforms"].as_array()) {
            for (auto& uniformNode : *uniformsArray) {
                auto uniformTable = uniformNode.as_table();
                if (!uniformTable) continue;

                UniformDeclaration uniform;
                uniform.name = (*uniformTable)["name"].value_or<std::string>("");
                uniform.type = (*uniformTable)["type"].value_or<std::string>("float");
                uniform.description = (*uniformTable)["description"].value_or<std::string>("");

                if (auto defVal = (*uniformTable)["default"].value<std::string>()) {
                    uniform.defaultValue = *defVal;
                }
                if (auto minVal = (*uniformTable)["min"].value<double>()) {
                    uniform.minValue = static_cast<float>(*minVal);
                }
                if (auto maxVal = (*uniformTable)["max"].value<double>()) {
                    uniform.maxValue = static_cast<float>(*maxVal);
                }

                metadata_.uniforms.push_back(uniform);
            }
        }

        if (auto texturesArray = config["textures"].as_array()) {
            for (auto& textureNode : *texturesArray) {
                auto textureTable = textureNode.as_table();
                if (!textureTable) continue;

                TextureInput texture;
                texture.name = (*textureTable)["name"].value_or<std::string>("");
                texture.description = (*textureTable)["description"].value_or<std::string>("");
                if (auto defVal = (*textureTable)["default"].value<std::string>()) {
                    texture.defaultValue = *defVal;
                }

                metadata_.textures.push_back(texture);
            }
        }

    } catch (const toml::parse_error& err) {
        throw std::runtime_error("Failed to parse TOML: " + std::string(err.what()));
    }
}

void Module::loadShader(const std::filesystem::path& shaderFile) {
    std::ifstream file(shaderFile);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + shaderFile.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    shaderSource_ = buffer.str();

    if (shaderSource_.empty()) {
        throw std::runtime_error("Shader file is empty: " + shaderFile.string());
    }
}

void Module::loadPasses(const std::filesystem::path& baseDir) {
    try {
        auto tomlPath = baseDir / (metadata_.name + ".toml");
        auto config = toml::parse_file(tomlPath.string());

        if (auto passesArray = config["passes"].as_array()) {
            for (auto& passNode : *passesArray) {
                auto passTable = passNode.as_table();
                if (!passTable) continue;

                Pass pass;
                pass.name = (*passTable)["name"].value_or<std::string>("");
                pass.file = (*passTable)["file"].value_or<std::string>("");

                if (pass.file.empty()) {
                    throw std::runtime_error("Pass missing 'file' field");
                }

                auto passShaderFile = baseDir / pass.file;
                std::ifstream file(passShaderFile);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open pass shader file: " + passShaderFile.string());
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                pass.shaderSource = buffer.str();

                if (pass.shaderSource.empty()) {
                    throw std::runtime_error("Pass shader file is empty: " + passShaderFile.string());
                }

                metadata_.passes.push_back(pass);
            }
        }

        if (metadata_.passes.empty()) {
            throw std::runtime_error("Stage effect must have at least one pass");
        }

    } catch (const toml::parse_error& err) {
        throw std::runtime_error("Failed to parse passes: " + std::string(err.what()));
    }
}

std::string Module::getProcessedShader() const {
    return shaderSource_;
}

} // namespace kaisei::core
