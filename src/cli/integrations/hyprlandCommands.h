#pragma once

#include <CLI/CLI.hpp>

namespace kaisei::cli {

class HyprlandCommands {
public:
    static void setup(CLI::App* app);

private:
    static void on();
    static void off();
    static void toggle();
    static void status();
    static void reload();

    static std::string sendCommand(const std::string& command);
};

} // namespace kaisei::cli
