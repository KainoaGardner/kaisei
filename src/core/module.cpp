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

    auto shaderFile = modulePath_ / metadata_.name;
    loadShader(shaderFile);

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
        metadata_.fusible = shader["fusible"].value_or(true);
        metadata_.fusionPriority = shader["fusion_priority"].value_or(0);
        metadata_.inputVariable = shader["input_variable"].value_or<std::string>("inputColor");
        metadata_.outputVariable = shader["output_variable"].value_or<std::string>("fragColor");

        if (config["dependencies"]) {
            auto deps = config["dependencies"];
            if (auto requiredArray = deps["requires"].as_array()) {
                for (auto& req : *requiredArray) {
                    metadata_.requiredModules.push_back(req.value_or<std::string>(""));
                }
            }
            if (auto conflictsArray = deps["conflicts"].as_array()) {
                for (auto& conf : *conflictsArray) {
                    metadata_.conflictsWith.push_back(conf.value_or<std::string>(""));
                }
            }
        }

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
                texture.optional = (*textureTable)["optional"].value_or(false);

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

std::string Module::getProcessedShader() const {
    // TODO: Phase 1 - simple pass-through
    // TODO: Phase 2 - resolve #include directives from shaders/common.glsl
    return shaderSource_;
}

} // namespace kaisei::core
