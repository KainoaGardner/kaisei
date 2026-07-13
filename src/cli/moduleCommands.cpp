#include "cli/moduleCommands.h"
#include "utils/format.h"
#include "backend/backend.h"
#include "backend/openGLBackend.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "utils/shaderErrorFormatter.h"

namespace kaisei::cli {

void ModuleCommands::setup(CLI::App* module, core::Registry& registry) {
    // module list (--tag)
    auto* list = module->add_subcommand("list", "List available modules");
    static std::string tag;

    list->add_option("--tag,-t", tag, "Filter by tag (optional)");
    list->callback([&]() { ModuleCommands::list(registry, tag); });

    // module info
    auto* info = module->add_subcommand("info", "Show module details");
    static std::string infoName;

    info->add_option("name", infoName, "Module name")->required();
    info->callback([&]() { ModuleCommands::info(registry, infoName); });

    // module create
    auto* create = module->add_subcommand("create", "Create a new module");
    static std::string createName;
    static std::string version = "1.0";
    static std::string description = "";
    static bool stage = false;

    create->add_option("name", createName, "Module name")->required();
    create->add_option("--version,-v", version, "Module version (default: 1.0)");
    create->add_option("--description,-d", description, "Module description");
    create->add_flag("--stage,-s", stage, "Create a multi-pass stage module instead of single-pass");
    create->callback([&]() { ModuleCommands::create(registry, createName, version, description, stage); });

    // module delete
    auto* del = module->add_subcommand("delete", "Delete a module");
    static std::string deleteName;

    del->add_option("name", deleteName, "Module name")->required();
    del->callback([&]() { ModuleCommands::deleteModule(registry, deleteName); });

    // module compile
    auto* compile = module->add_subcommand("compile", "Test compile a shader file headlessly");
    static std::string shaderPath;
    compile->add_option("file", shaderPath, "Path to the shader file (.frag)")->required();
    compile->callback([&]() { ModuleCommands::compileShader(shaderPath); });
}

void ModuleCommands::compileShader(const std::string& shaderPath) {
    std::cout << "Testing compilation of shader: " << shaderPath << "...\n";

    std::ifstream file(shaderPath);
    if (!file.is_open()) {
        spdlog::error("Failed to open shader file: {}", shaderPath);
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    if (!glfwInit()) {
        spdlog::error("Failed to initialize GLFW");
        return;
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1, 1, "Headless", nullptr, nullptr);
    if (!window) {
        spdlog::error("Failed to create headless window");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        spdlog::error("Failed to initialize GLAD");
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }

    try {
        std::unique_ptr<backend::Backend> backend = std::make_unique<backend::OpenGLBackend>();
        if (!backend->initialize()) {
            spdlog::error("Failed to initialize backend");
            glfwDestroyWindow(window);
            glfwTerminate();
            return;
        }

        uint32_t shader = backend->compileShader(source, backend::Backend::FRAGMENT_SHADER);
        
        std::cout << "\033[1;32mShader compiled successfully!\033[0m\n";
    } catch (const std::runtime_error& e) {
        std::string errStr = e.what();
        if (errStr.find("Shader compilation failed:") == 0) {
            std::string logMsg = errStr.substr(26);
            std::string formatted = utils::formatShaderError(logMsg, source, "module-compiler", shaderPath);
            std::cout << formatted << "\n";
        } else {
            spdlog::error("Compilation failed: {}", e.what());
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

void ModuleCommands::list(core::Registry& registry, const std::string& tag) {
    auto modules = registry.modules().listModules();

    if (modules.empty()) {
        std::cout << "No modules found\n";
        return;
    }

    std::vector<std::pair<std::string, const core::Module*>> filteredModules;

    for (const auto& name : modules) {
        auto* module = registry.modules().getModule(name);
        const auto& metadata = module->metadata();

        if (!tag.empty()) {
            bool matches = false;
            for (const auto& moduleTag : metadata.tags) {
                if (moduleTag == tag) {
                    matches = true;
                    break;
                }
            }
            if (!matches) continue;
        }

        filteredModules.push_back({name, module});
    }

    if (filteredModules.empty()) {
        std::cout << "No modules found\n";
        return;
    }

    std::sort(filteredModules.begin(), filteredModules.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    std::cout << utils::bold("---Modules---") << "\n";
    if (!tag.empty()) {
        std::cout << utils::bold("Tag:")  << "'" << tag << "'\n";
    }

    size_t i = 0;
    for (const auto& [name, module] : filteredModules) {
        const auto& metadata = module->metadata();
        std::cout << utils::bold(std::to_string(i++) + ". ") << metadata.name << " " << metadata.version << "\n";
    }
}

void ModuleCommands::info(core::Registry& registry, const std::string& name) {
    if (!registry.modules().hasModule(name)) {
        spdlog::error("Module '{}' not found", name);
        return;
    }

    auto* module = registry.modules().getModule(name);
    const auto& metadata = module->metadata();

    std::cout << utils::bold("Module: ") << metadata.name << "\n";
    std::cout << utils::bold("Version: ") << metadata.version << "\n";
    std::cout << utils::bold("Author: ") << metadata.author << "\n";
    std::cout << utils::bold("Description: ") << metadata.description << "\n";
    std::cout << utils::bold("Type: ") << (metadata.type == core::EffectType::Module ? "module" : "stage") << "\n";

    if (metadata.type == core::EffectType::Stage && !metadata.passes.empty()) {
        std::cout << utils::bold("Passes: ");
        for (size_t i = 0; i < metadata.passes.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << metadata.passes[i].name;
        }
        std::cout << "\n";
    }

    if (!metadata.tags.empty()) {
        std::cout << utils::bold("Tags: ");
        for (size_t i = 0; i < metadata.tags.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << metadata.tags[i];
        }
        std::cout << "\n";
    }

    if (!metadata.uniforms.empty()) {
        std::cout << utils::bold("\nUniforms:\n");
        for (const auto& uniform : metadata.uniforms) {
            std::cout << "  " << uniform.name << " (" << uniform.type << ")";
            if (uniform.defaultValue) {
                std::cout << " = " << *uniform.defaultValue;
            }
            std::cout << "\n";
            if (!uniform.description.empty()) {
                std::cout << "    " << uniform.description << "\n";
            }
        }
    }

    if (!metadata.textures.empty()) {
        std::cout << utils::bold("\nTextures:\n");
        for (const auto& texture : metadata.textures) {
            std::cout << "  " << texture.name;
            if (texture.optional) {
                std::cout << " (optional)";
            }
            std::cout << "\n";
            if (!texture.description.empty()) {
                std::cout << "    " << texture.description << "\n";
            }
        }
    }
}

void ModuleCommands::create(core::Registry& registry, const std::string& name,
                            const std::string& version, const std::string& description, bool stage) {
    try {
        registry.modules().createModule(name, version, description, stage);
        registry.modules().reload();

        std::cout << utils::bold("Created module: ") << name << (stage ? " (multi-pass)" : "") << "\n";
        std::cout << "Files created in ~/.config/kaisei/modules/:\n";
        if (stage) {
            std::cout << "  - " << name << "/" << name << ".toml\n";
            std::cout << "  - " << name << "/" << name << "_pass1.frag\n";
            std::cout << "  - " << name << "/" << name << "_pass2.frag\n";
        } else {
            std::cout << "  - " << name << ".toml\n";
            std::cout << "  - " << name << ".frag\n";
        }
        std::cout << "\nEdit these files to customize your module, then run " << utils::bold("kaisei reload") << " to apply changes.\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to create module: {}", e.what());
    }
}

void ModuleCommands::deleteModule(core::Registry& registry, const std::string& name) {
    try {
        registry.modules().deleteModule(name);
        registry.modules().reload();
        std::cout << utils::bold("Deleted module: ") << name << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to delete module: {}", e.what());
    }
}

} // namespace kaisei::cli
