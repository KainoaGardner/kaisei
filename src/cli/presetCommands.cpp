#include "cli/presetCommands.h"
#include "utils/format.h"

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
 
    // preset delete <name>
    auto* del = preset->add_subcommand("delete", "Delete a preset");
    static std::string delPresetName;

    del->add_option("name", delPresetName, "Preset name")->required();
    del->callback([&]() { PresetCommands::deletePreset(registry, delPresetName); });

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

    // preset move <name> <from> <to>
    auto* move = preset->add_subcommand("move", "Move a module within a preset");
    static std::string mvPresetName;
    static size_t fromIndex;
    static size_t toIndex;

    move->add_option("preset", mvPresetName, "Preset name")->required();
    move->add_option("from", fromIndex, "Current module index")->required();
    move->add_option("to", toIndex, "Target module index")->required();
    move->callback([&]() { PresetCommands::move(registry, mvPresetName, fromIndex, toIndex); });

    // preset list
    auto* list = preset->add_subcommand("list", "List all presets");
    list->callback([&]() { PresetCommands::list(registry); });

    // preset show <name>
    auto* show = preset->add_subcommand("show", "Show preset details");
    static std::string showName;

    show->add_option("name", showName, "Preset name")->required();
    show->callback([&]() { PresetCommands::show(registry, showName); });

    // preset current
    auto* current = preset->add_subcommand("current", "Show current preset");
    current->callback([&]() { PresetCommands::current(registry); });

    // preset select <name>
    auto* select = preset->add_subcommand("select", "Select a preset as current");
    static std::string selectName;

    select->add_option("name", selectName, "Preset name")->required();
    select->callback([&]() { PresetCommands::select(registry, selectName); });

    // preset unselect
    auto* unselect = preset->add_subcommand("unselect", "Clear current preset selection");
    unselect->callback([&]() { PresetCommands::unselect(registry); });

    // preset next
    auto* next = preset->add_subcommand("next", "Cycle to next preset");
    next->callback([&]() { PresetCommands::next(registry); });

    // preset prev
    auto* prev = preset->add_subcommand("prev", "Cycle to previous preset");
    prev->callback([&]() { PresetCommands::prev(registry); });

    // preset export <name> <path>
    auto* exp = preset->add_subcommand("export", "Export a preset and its modules as a bundle");
    static std::string exportPresetName;
    static std::string exportPath;
    exp->add_option("preset", exportPresetName, "Preset name to export")->required();
    exp->add_option("path", exportPath, "Destination bundle directory path")->required();
    exp->callback([&]() { PresetCommands::exportPreset(registry, exportPresetName, exportPath); });

    // preset import <path>
    auto* imp = preset->add_subcommand("import", "Import a preset bundle or file");
    static std::string importPath;
    imp->add_option("path", importPath, "Path to the bundle directory or preset .toml file")->required();
    imp->callback([&]() { PresetCommands::importPreset(registry, importPath); });
}

void PresetCommands::create(core::Registry& registry, const std::string& name, const std::string& version, const std::string& description) {
    auto* preset = registry.presets().createPreset(name, version, description);
    registry.presets().reload();

    std::cout << utils::bold("Created preset: ") << name << " " << version << "\n";
    if (!description.empty()) {
        std::cout << utils::bold("Description: ") << description << "\n";
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

    std::cout << utils::bold("Added module") << " '" << module << "' " <<  utils::bold("to preset") << " '" << name << "'\n";
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

    std::cout << utils::bold("Removed module at index ") << index << utils::bold(" from preset") << " '" << name << "'\n";
}

void PresetCommands::move(core::Registry& registry, const std::string& name, size_t fromIndex, size_t toIndex) {
    if (!registry.presets().hasPreset(name)) {
        spdlog::error("Preset '{}' not found", name);
        return;
    }

    auto* preset = registry.presets().getPresetMutable(name);

    if (fromIndex >= preset->modules().size()) {
        spdlog::error("From index {} out of range (preset has {} modules)", fromIndex, preset->modules().size());
        return;
    }

    if (toIndex >= preset->modules().size()) {
        spdlog::error("To index {} out of range (preset has {} modules)", toIndex, preset->modules().size());
        return;
    }

    preset->moveModule(fromIndex, toIndex);
    registry.presets().save(name);

    std::cout << utils::bold("Moved module from index ") << fromIndex << utils::bold(" to index ") << toIndex
              << utils::bold(" in preset") << " '" << name << "'\n";
}

void PresetCommands::deletePreset(core::Registry& registry, const std::string& name) {
    if (!registry.presets().hasPreset(name)) {
        spdlog::error("Preset '{}' not found", name);
        return;
    }

    registry.presets().deletePreset(name);
    registry.presets().reload();
    std::cout << utils::bold("Deleted preset: ") << "'" << name << "'\n";
}

void PresetCommands::list(core::Registry& registry) {
    auto presets = registry.presets().listPresets();

    if (presets.empty()) {
        std::cout << "No presets found\n";
        return;
    }

    std::string currentPreset = registry.getCurrentPreset();

    size_t i = 0;
    std::cout << utils::bold("Available presets:\n");
    for (const auto& name : presets) {
        auto* preset = registry.presets().getPreset(name);
        std::string marker = (name == currentPreset) ? " *" : "  ";
        std::cout << marker << utils::bold(std::to_string(i++) + ". ") << name << " " << preset->version()
                  << utils::bold(" - ") << utils::bold(std::to_string(preset->modules().size())) << utils::bold(" modules\n");
    }
}

void PresetCommands::show(core::Registry& registry, const std::string& name) {
    if (!registry.presets().hasPreset(name)) {
        spdlog::error("Preset '{}' not found", name);
        return;
    }

    auto* preset = registry.presets().getPreset(name);

    std::cout << utils::bold("Preset: ") << preset->name() << "\n";
    std::cout << utils::bold("Version: ") << preset->version() << "\n";
    if (!preset->description().empty()) {
        std::cout << utils::bold("Description: ") << preset->description() << "\n";
    }
    std::cout << utils::bold("\nModules: ") << utils::bold(std::to_string(preset->modules().size())) << "\n";

    size_t index = 0;
    for (const auto& module : preset->modules()) {
        std::cout << "  [" << index++ << "] " << module.moduleName;
        std::cout << "\n";
    }

    std::map<std::string, std::string> totalOverrides;
    for (const auto& module : preset->modules()) {
        for (const auto& [key, value] : module.uniformOverrides) {
            totalOverrides[key] = value;
        }
    }

    if (!totalOverrides.empty()) {
        std::cout << utils::bold("\nOverrides:\n");
        for (const auto& [key, value] : totalOverrides) {
            std::cout << "  " << key << " = " << value << "\n";
        }
    }
}

void PresetCommands::current(core::Registry& registry) {
    std::string currentPreset = registry.getCurrentPreset();

    if (currentPreset.empty()) {
        std::cout << "No preset currently selected\n";
        return;
    }

    if (!registry.presets().hasPreset(currentPreset)) {
        std::cout << utils::bold("Current preset: ") << currentPreset << " " << utils::bold("(not found)\n");
        return;
    }

    auto* preset = registry.presets().getPreset(currentPreset);
    std::cout << utils::bold("Current preset: ") << currentPreset << " " << preset->version() << "\n";
}

void PresetCommands::select(core::Registry& registry, const std::string& name) {
    if (!registry.presets().hasPreset(name)) {
        spdlog::error("Preset '{}' not found", name);
        return;
    }

    registry.setCurrentPreset(name);
    auto* preset = registry.presets().getPreset(name);
    std::cout << utils::bold("Selected preset: ") << name << " " << preset->version() << "\n";
}

void PresetCommands::unselect(core::Registry& registry) {
    std::string currentPreset = registry.getCurrentPreset();

    if (currentPreset.empty()) {
        std::cout << "No preset currently selected\n";
        return;
    }

    registry.clearCurrentPreset();
    std::cout << utils::bold("Cleared preset selection") << " (was: " << currentPreset << ")\n";
}

void PresetCommands::next(core::Registry& registry) {
    try {
        std::string preset = registry.nextPreset();
        std::cout << utils::bold("Next preset: ") << preset << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to cycle to next preset: {}", e.what());
    }
}

void PresetCommands::prev(core::Registry& registry) {
    try {
        std::string preset = registry.prevPreset();
        std::cout << utils::bold("Previous preset: ") << preset << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to cycle to previous preset: {}", e.what());
    }
}

void PresetCommands::exportPreset(core::Registry& registry, const std::string& name, const std::string& path) {
    try {
        registry.presets().exportPreset(name, path, registry.modules());
        std::cout << utils::bold("Successfully exported preset bundle '") << name << "' to " << path << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Export failed: {}", e.what());
    }
}

void PresetCommands::importPreset(core::Registry& registry, const std::string& path) {
    try {
        registry.presets().importPreset(path, registry.modules());
        registry.reload();
        std::cout << utils::bold("Successfully imported preset bundle/file from ") << path << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Import failed: {}", e.what());
    }
}

} // namespace kaisei::cli
