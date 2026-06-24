#include "cli/presetCommands.h"

#include <spdlog/spdlog.h>
#include <iostream>

namespace kaisei::cli {

void PresetCommands::setup(CLI::App* preset, core::Registry& registry) {
    // preset create <name>
    auto* create = preset->add_subcommand("create", "Create a new preset");
    static std::string name;
    static std::string version = "1.0";
    static std::string description;

    create->add_option("name", name, "Preset name")->required();
    create->add_option("--version,-v", version, "Preset version");
    create->add_option("--description,-d", description, "Preset description");
    create->callback([&]() { PresetCommands::create(registry, name, version, description); });

    // preset add <name> <module>
    auto* add = preset->add_subcommand("add", "Add a module to a preset");
    static std::string presetName;
    static std::string moduleName;
    static std::vector<std::string> overrides;

    add->add_option("preset", presetName, "Preset name")->required();
    add->add_option("module", moduleName, "Module name")->required();
    add->add_option("--override,-o", overrides, "Uniform overrides (key=value)");
    add->callback([&]() { PresetCommands::add(registry, presetName, moduleName, overrides); });

    // preset remove <name> <index>
    auto* remove = preset->add_subcommand("remove", "Remove a module from a preset");
    static std::string rmPresetName;
    static size_t index;

    remove->add_option("preset", rmPresetName, "Preset name")->required();
    remove->add_option("index", index, "Module index to remove")->required();
    remove->callback([&]() { PresetCommands::remove(registry, rmPresetName, index); });

    // preset list
    auto* list = preset->add_subcommand("list", "List all presets");
    list->callback([&]() { PresetCommands::list(registry); });

    // preset show <name>
    auto* show = preset->add_subcommand("show", "Show preset details");
    static std::string showName;

    show->add_option("name", showName, "Preset name")->required();
    show->callback([&]() { PresetCommands::show(registry, showName); });
}

void PresetCommands::create(core::Registry& registry, const std::string& name, const std::string& version, const std::string& description) {
    auto* preset = registry.presets().createPreset(name, version, description);

    std::cout << "Created preset: " << preset->name() << " (v" << preset->version() << ")\n";
    if (!description.empty()) {
        std::cout << "Description: " << description << "\n";
    }
}

void PresetCommands::add(core::Registry& registry, const std::string& name, const std::string& module, const std::vector<std::string>& overrides) {
    if (!registry.presets().hasPreset(name)) {
        spdlog::error("Preset '{}' not found", name);
        return;
    }

    if (!registry.modules().hasModule(module)) {
        spdlog::error("Module '{}' not found", module);
        return;
    }

    // Parse overrides (key=value format)
    std::map<std::string, std::string> overrideMap;
    for (const auto& override : overrides) {
        size_t pos = override.find('=');
        if (pos == std::string::npos) {
            spdlog::warn("Invalid override format '{}', expected key=value", override);
            continue;
        }
        std::string key = override.substr(0, pos);
        std::string value = override.substr(pos + 1);
        overrideMap[key] = value;
    }

    auto* preset = registry.presets().getPresetMutable(name);
    preset->addModule(module, overrideMap);
    registry.presets().save(name);

    std::cout << "Added module '" << module << "' to preset '" << name << "'\n";
}

void PresetCommands::remove(core::Registry& registry, const std::string& name, size_t index) {
    if (!registry.presets().hasPreset(name)) {
        spdlog::error("Preset '{}' not found", name);
        return;
    }

    auto* preset = registry.presets().getPresetMutable(name);

    if (index >= preset->modules().size()) {
        spdlog::error("Module index {} out of range (preset has {} modules)", index, preset->modules().size());
        return;
    }

    preset->removeModule(index);
    registry.presets().save(name);

    std::cout << "Removed module at index " << index << " from preset '" << name << "'\n";
}

void PresetCommands::list(core::Registry& registry) {
    auto presets = registry.presets().listPresets();

    if (presets.empty()) {
        std::cout << "No presets found\n";
        return;
    }

    std::cout << "Available presets:\n";
    for (const auto& name : presets) {
        auto* preset = registry.presets().getPreset(name);
        std::cout << "  " << name << " (v" << preset->version() << ") - "
                  << preset->modules().size() << " modules\n";
        if (!preset->description().empty()) {
            std::cout << "    " << preset->description() << "\n";
        }
    }
}

void PresetCommands::show(core::Registry& registry, const std::string& name) {
    if (!registry.presets().hasPreset(name)) {
        spdlog::error("Preset '{}' not found", name);
        return;
    }

    auto* preset = registry.presets().getPreset(name);

    std::cout << "Preset: " << preset->name() << "\n";
    std::cout << "Version: " << preset->version() << "\n";
    if (!preset->description().empty()) {
        std::cout << "Description: " << preset->description() << "\n";
    }
    std::cout << "\nModules (" << preset->modules().size() << "):\n";

    size_t index = 0;
    for (const auto& module : preset->modules()) {
        std::cout << "  [" << index++ << "] " << module.moduleName;
        if (!module.uniformOverrides.empty()) {
            std::cout << " (overrides: ";
            bool first = true;
            for (const auto& [key, value] : module.uniformOverrides) {
                if (!first) std::cout << ", ";
                std::cout << key << "=" << value;
                first = false;
            }
            std::cout << ")";
        }
        std::cout << "\n";
    }
}

} // namespace kaisei::cli
