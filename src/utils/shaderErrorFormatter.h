#pragma once

#include <string>

namespace kaisei::utils {

std::string formatShaderError(const std::string& logMessage, 
                              const std::string& source, 
                              const std::string& moduleName, 
                              const std::string& shaderPath);

} // namespace kaisei::utils
