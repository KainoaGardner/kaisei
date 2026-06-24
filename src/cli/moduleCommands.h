#pragma once

#include <CLI/CLI.hpp>
#include "core/registry.h"

namespace kaisei::cli {

class ModuleCommands {
public:
    static void setup(CLI::App* module, core::Registry& registry);

private:
    static void list(core::Registry& registry, const std::string& tag);
    static void info(core::Registry& registry, const std::string& name);
};

} // namespace kaisei::cli
