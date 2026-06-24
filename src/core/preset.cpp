#include "core/preset.h"

#include <stdexcept>

#include <spdlog/spdlog.h>

namespace kaisei::core {

Preset::Preset(const std::string& name, const std::string& version)
    : name_(name), version_(version) {
}

void Preset::addModule(const std::string& moduleName,
                       const std::map<std::string, std::string>& overrides) {
    PresetModule module;
    module.moduleName = moduleName;
    module.uniformOverrides = overrides;
    modules_.push_back(module);

    spdlog::debug("Added module '{}' to preset '{}'", moduleName, name_);
}

void Preset::removeModule(size_t index) {
    if (index >= modules_.size()) {
        throw std::out_of_range("Module index out of range");
    }
    modules_.erase(modules_.begin() + index);
    spdlog::debug("Removed module at index {} from preset '{}'", index, name_);
}

void Preset::moveModule(size_t from, size_t to) {
    if (from >= modules_.size() || to >= modules_.size()) {
        throw std::out_of_range("Module index out of range");
    }

    if (from == to) return;

    auto module = modules_[from];
    modules_.erase(modules_.begin() + from);
    modules_.insert(modules_.begin() + to, module);

    spdlog::debug("Moved module from index {} to {} in preset '{}'", from, to, name_);
}

} // namespace kaisei::core
