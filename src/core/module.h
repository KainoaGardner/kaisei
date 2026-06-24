#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace kaisei::core {

enum class EffectType {
    Module,  // Simple 1 pass effect
    Stage    // Multi-pass effect
};

struct UniformDeclaration {
    std::string name;
    std::string type;
    std::optional<std::string> defaultValue;
    std::string description;
    std::optional<float> minValue;
    std::optional<float> maxValue;
};

struct TextureInput {
    std::string name;
    std::string description;
    bool optional;
};

struct ModuleMetadata {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::vector<std::string> tags;
    EffectType type;

    std::vector<std::string> requiredModules;
    std::vector<std::string> conflictsWith;

    std::vector<UniformDeclaration> uniforms;
    std::vector<TextureInput> textures;

    std::string shaderFile;
    bool fusible;
    int fusionPriority;
    std::string inputVariable;
    std::string outputVariable;
};

class Module {
public:
    explicit Module(const std::filesystem::path& moduleFile);

    const ModuleMetadata& metadata() const { return metadata_; }
    const std::string& shaderSource() const { return shaderSource_; }
    const std::filesystem::path& path() const { return modulePath_; }

    std::string getProcessedShader() const;

private:
    void loadMetadata(const std::filesystem::path& tomlFile);
    void loadShader(const std::filesystem::path& shaderFile);

    ModuleMetadata metadata_;
    std::string shaderSource_;
    std::filesystem::path modulePath_;
};

} // namespace kaisei::core
