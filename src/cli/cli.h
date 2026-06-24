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
    void setupPresetCommands();
    void setupModuleCommands();

    std::unique_ptr<CLI::App> app_;
    core::Registry registry_;
};

} // namespace kaisei::cli
