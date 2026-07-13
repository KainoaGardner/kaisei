#include "utils/shaderErrorFormatter.h"

#include <vector>
#include <sstream>
#include <regex>
#include <algorithm>

namespace kaisei::utils {

std::string formatShaderError(const std::string& logMessage, 
                              const std::string& source, 
                              const std::string& moduleName, 
                              const std::string& shaderPath) {
    std::string formatted = "Shader compilation failed in module '" + moduleName + "'\n";
    if (!shaderPath.empty()) {
        formatted += "Shader file: " + shaderPath + "\n";
    }
    formatted += "\n--- Compiler Errors ---\n";

    std::vector<std::string> sourceLines;
    {
        std::stringstream ss(source);
        std::string line;
        while (std::getline(ss, line)) {
            sourceLines.push_back(line);
        }
    }

    std::stringstream logSS(logMessage);
    std::string logLine;
    
    std::regex mesaRegex(R"(\b\d+:(\d+)(?:\(\d+\))?:)");
    std::regex nvidiaRegex(R"(\b\d+\((\d+)\)\s*:)");
    std::regex appleRegex(R"(ERROR:\s+\d+:(\d+):)");

    while (std::getline(logSS, logLine)) {
        formatted += logLine + "\n";

        std::smatch match;
        int lineNum = -1;

        if (std::regex_search(logLine, match, mesaRegex) && match.size() > 1) {
            lineNum = std::stoi(match[1].str());
        } else if (std::regex_search(logLine, match, nvidiaRegex) && match.size() > 1) {
            lineNum = std::stoi(match[1].str());
        } else if (std::regex_search(logLine, match, appleRegex) && match.size() > 1) {
            lineNum = std::stoi(match[1].str());
        }

        if (lineNum > 0 && lineNum <= static_cast<int>(sourceLines.size())) {
            formatted += "  --> Context around line " + std::to_string(lineNum) + ":\n";
            int startCtx = std::max(1, lineNum - 2);
            int endCtx = std::min(static_cast<int>(sourceLines.size()), lineNum + 2);
            for (int i = startCtx; i <= endCtx; ++i) {
                if (i == lineNum) {
                    formatted += "    > " + std::to_string(i) + " | " + sourceLines[i - 1] + "   <-- ERROR\n";
                } else {
                    formatted += "      " + std::to_string(i) + " | " + sourceLines[i - 1] + "\n";
                }
            }
            formatted += "\n";
        }
    }

    return formatted;
}

} // namespace kaisei::utils
