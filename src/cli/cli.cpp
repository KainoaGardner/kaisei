#include "cli/cli.h"
#include "cli/presetCommands.h"
#include "cli/moduleCommands.h"

#include <spdlog/spdlog.h>

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
    setupModuleCommands();
    setupPresetCommands();
}

void CLIApp::setupGlobalOptions() {
    app_->add_flag_callback("--verbose,-v",
        []() { spdlog::set_level(spdlog::level::debug);},
        "Enable verbose logging");
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
