#include "cli/moduleCommands.h"
#include "utils/format.h"

#include <spdlog/spdlog.h>
#include <iostream>
#include <string>

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

    create->add_option("name", createName, "Module name")->required();
    create->add_option("--version,-v", version, "Module version (default: 1.0)");
    create->add_option("--description,-d", description, "Module description");
    create->callback([&]() { ModuleCommands::create(registry, createName, version, description); });

    // module delete
    auto* del = module->add_subcommand("delete", "Delete a module");
    static std::string deleteName;

    del->add_option("name", deleteName, "Module name")->required();
    del->callback([&]() { ModuleCommands::deleteModule(registry, deleteName); });
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
                            const std::string& version, const std::string& description) {
    try {
        registry.modules().createModule(name, version, description);
        std::cout << utils::bold("Created module: ") << name << "\n";
        std::cout << "Files created in ~/.config/kaisei/modules/:\n";
        std::cout << "  - " << name << ".toml\n";
        std::cout << "  - " << name << ".frag\n";
        std::cout << "\nEdit these files to customize your module.\n";
        std::cout << "Then reload modules with: " << utils::bold("kaisei reload") << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to create module: {}", e.what());
    }
}

void ModuleCommands::deleteModule(core::Registry& registry, const std::string& name) {
    try {
        registry.modules().deleteModule(name);
        std::cout << utils::bold("Deleted module: ") << name << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to delete module: {}", e.what());
    }
}

} // namespace kaisei::cli
