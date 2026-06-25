#pragma once

#include <string>

namespace kaisei::utils {

inline std::string bold(const std::string& text) {
    return "\033[1m" + text + "\033[0m";
}

} // namespace kaisei::utils
