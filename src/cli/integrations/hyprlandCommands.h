#pragma once

#include <CLI/CLI.hpp>
#include "core/registry.h"

namespace kaisei::cli {

class HyprlandCommands {
public:
    static void setup(CLI::App* app, core::Registry& registry);

private:
    static void load(core::Registry& registry, const std::string& presetName);
    static void on();
    static void off();
    static void toggle();
    static void status();

    static std::string sendCommand(const std::string& command);
};

} // namespace kaisei::cli
