#pragma once

#include <CLI/CLI.hpp>
#include "core/registry.h"

namespace kaisei::cli {

class PresetCommands {
public:
    static void setup(CLI::App* preset, core::Registry& registry);

private:
    static void create(core::Registry& registry, const std::string& name, const std::string& version, const std::string& description);
    static void deletePreset(core::Registry& registry, const std::string& name);
    static void add(core::Registry& registry, const std::string& name, const std::string& module, const std::vector<std::string>& overrides);
    static void remove(core::Registry& registry, const std::string& name, size_t index);
    static void move(core::Registry& registry, const std::string& name, size_t fromIndex, size_t toIndex);
    static void list(core::Registry& registry);
    static void show(core::Registry& registry, const std::string& name);
    static void current(core::Registry& registry);
    static void select(core::Registry& registry, const std::string& name);
    static void unselect(core::Registry& registry);
    static void next(core::Registry& registry);
    static void prev(core::Registry& registry);
    static void exportPreset(core::Registry& registry, const std::string& name, const std::string& path);
    static void importPreset(core::Registry& registry, const std::string& path);
};

} // namespace kaisei::cli
