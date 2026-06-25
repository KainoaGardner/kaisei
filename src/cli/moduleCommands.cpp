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
    static std::string name;

    info->add_option("name", name, "Module name")->required();
    info->callback([&]() { ModuleCommands::info(registry, name); });
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

} // namespace kaisei::cli
