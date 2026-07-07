#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace kaisei::core {

enum class EffectType {
    Module,  // Simple 1 pass
    Stage    // Multi-pass
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

struct Pass {
    std::string name;
    std::string file;
    std::string shaderSource;
};

struct ModuleMetadata {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::vector<std::string> tags;
    EffectType type;

    std::vector<UniformDeclaration> uniforms;
    std::vector<TextureInput> textures;

    std::string shaderFile;

    std::vector<Pass> passes;
};

class Module {
public:
    explicit Module(const std::filesystem::path& moduleFile);

    const ModuleMetadata& metadata() const { return metadata_; }
    const std::string& shaderSource() const { return shaderSource_; }
    const std::filesystem::path& path() const { return modulePath_; }

    bool isMultiPass() const { return metadata_.type == EffectType::Stage; }
    const std::vector<Pass>& passes() const { return metadata_.passes; }

    std::string getProcessedShader() const;

private:
    void loadMetadata(const std::filesystem::path& tomlFile);
    void loadShader(const std::filesystem::path& shaderFile);
    void loadPasses(const std::filesystem::path& baseDir);

    ModuleMetadata metadata_;
    std::string shaderSource_;
    std::filesystem::path modulePath_;
};

} // namespace kaisei::core
