#pragma once

#include <CLI/CLI.hpp>
#include "core/registry.h"

namespace kaisei::cli {

class PreviewCommands {
public:
    static void setup(CLI::App* preview, core::Registry& registry);

private:
    static void start(core::Registry& registry, const std::string& imagePath);
    static void preview(core::Registry& registry, const std::string& imagePath, const std::string& presetName);
};

} // namespace kaisei::cli
