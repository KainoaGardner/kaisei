#pragma once

#include <memory>

#include <CLI/CLI.hpp>
#include "core/registry.h"

namespace kaisei::cli {

class CLIApp {
public:
    CLIApp();

    int run(int argc, char** argv);

private:
    void setupCommands();
    void setupGlobalOptions();
    void setupReloadCommand();
    void setupPresetCommands();
    void setupModuleCommands();
    void setupPreviewCommands();
    void setupHyprlandCommands();

    std::unique_ptr<CLI::App> app_;
    core::Registry registry_;
};

} // namespace kaisei::cli
