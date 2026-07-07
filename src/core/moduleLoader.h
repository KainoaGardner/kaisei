#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/module.h"

namespace kaisei::core {

class ModuleLoader {
public:
    explicit ModuleLoader(const std::vector<std::filesystem::path>& searchPaths = {});

    void addSearchPath(const std::filesystem::path& path);
    void setStandardSavePath(const std::filesystem::path& path);
    void scanModules();

    const Module* getModule(const std::string& name) const;
    bool hasModule(const std::string& name) const;

    std::vector<std::string> listModules() const;
    std::vector<const Module*> findByTag(const std::string& tag) const;

    void createModule(const std::string& name,
                     const std::string& version = "1.0",
                     const std::string& description = "",
                     bool stage = false);
    void deleteModule(const std::string& name);

    void reload();
    void clear();

private:
    void loadModulesFromPath(const std::filesystem::path& path);
    std::filesystem::path getModuleBasePath(const std::string& name) const;

    std::vector<std::filesystem::path> searchPaths_;
    std::filesystem::path standardSavePath_;
    std::map<std::string, std::unique_ptr<Module>> modules_;
};

} // namespace kaisei::core
