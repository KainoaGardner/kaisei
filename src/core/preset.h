#pragma once

#include <map>
#include <string>
#include <vector>

namespace kaisei::core {

struct PresetModule {
    std::string moduleName;
    std::map<std::string, std::string> uniformOverrides;
};

class Preset {
public:
    explicit Preset(const std::string& name, const std::string& version = "1.0");

    void addModule(const std::string& moduleName,
                   const std::map<std::string, std::string>& overrides = {});
    void removeModule(size_t index);
    void moveModule(size_t from, size_t to);

    const std::vector<PresetModule>& modules() const { return modules_; }
    const std::string& name() const { return name_; }
    const std::string& version() const { return version_; }
    const std::string& description() const { return description_; }
    void setDescription(const std::string& desc) { description_ = desc; }
    void setVersion(const std::string& version) { version_ = version; }

private:
    std::string name_;
    std::string description_;
    std::string version_;
    std::vector<PresetModule> modules_;
};

} // namespace kaisei::core
