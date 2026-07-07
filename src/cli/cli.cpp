#include "cli/cli.h"
#include "cli/presetCommands.h"
#include "cli/moduleCommands.h"
#include "cli/integrations/previewCommands.h"
#include "cli/integrations/hyprlandCommands.h"
#include "utils/format.h"

#include <spdlog/spdlog.h>
#include <iostream>

namespace kaisei::cli {

CLIApp::CLIApp() {
    app_ = std::make_unique<CLI::App>("Kaisei - Modular shader composition framework");
    app_->require_subcommand(1);

    app_->parse_complete_callback([this]() {
        registry_.initialize();
    });

    setupCommands();
}

void CLIApp::setupCommands() {
    setupGlobalOptions();
    setupReloadCommand();
    setupModuleCommands();
    setupPresetCommands();
    setupPreviewCommands();
    setupHyprlandCommands();
}

void CLIApp::setupGlobalOptions() {
    app_->add_flag_callback("--verbose,-v",
        []() { spdlog::set_level(spdlog::level::debug);},
        "Enable verbose logging");
}

void CLIApp::setupReloadCommand() {
    auto* reload = app_->add_subcommand("reload", "Reload all modules and presets from disk");
    reload->callback([this]() {
        try {
            registry_.reload();
            std::cout << utils::bold("Reloaded modules and presets\n");

            auto modules = registry_.modules().listModules();
            auto presets = registry_.presets().listPresets();

            std::cout << "  Modules: " << modules.size() << "\n";
            std::cout << "  Presets: " << presets.size() << "\n";
        } catch (const std::exception& e) {
            spdlog::error("Failed to reload: {}", e.what());
        }
    });
}

void CLIApp::setupModuleCommands() {
    auto* module = app_->add_subcommand("module", "Manage modules");
    module->require_subcommand(1);

    ModuleCommands::setup(module, registry_);
}

void CLIApp::setupPresetCommands() {
    auto* preset = app_->add_subcommand("preset", "Manage presets");
    preset->require_subcommand(1);

    PresetCommands::setup(preset, registry_);
}

void CLIApp::setupPreviewCommands() {
    auto* preview = app_->add_subcommand("preview", "Preview presets on images");
    preview->require_subcommand(1);

    PreviewCommands::setup(preview, registry_);
}

void CLIApp::setupHyprlandCommands() {
    auto* hyprland = app_->add_subcommand("hyprland", "Control Hyprland integration");
    hyprland->require_subcommand(1);

    HyprlandCommands::setup(hyprland, registry_);
}

int CLIApp::run(int argc, char** argv) {
    try {
        app_->parse(argc, argv);

        return 0;
    } catch (const CLI::ParseError& e) {
        return app_->exit(e);
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }
}

} // namespace kaisei::cli
