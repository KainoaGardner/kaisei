#include "backend/shaderPreprocessor.h"

#include <spdlog/spdlog.h>

#include "backend/backend.h"

namespace kaisei::backend {

std::string ShaderPreprocessor::process(const std::string& source,
                                       BackendType backend,
                                       uint32_t shaderType) {
    switch (backend) {
        case BackendType::OpenGLES:
            return processOpenGLES(source, shaderType);
        case BackendType::OpenGL:
            return processOpenGL(source, shaderType);
        case BackendType::Vulkan:
            spdlog::warn("Vulkan backend not yet implemented");
            return source;
        default:
            return source;
    }
}

std::string ShaderPreprocessor::processOpenGLES(const std::string& source, uint32_t shaderType) {
    std::string result = source;

    result = replaceVersion(result, "#version 450 core", "#version 320 es");
    result = replaceVersion(result, "#version 330 core", "#version 320 es");
    result = replaceVersion(result, "#version 410 core", "#version 320 es");

    // Add precision for fragment shaders (Backend::FRAGMENT_SHADER)
    if (shaderType == Backend::FRAGMENT_SHADER) {
        if (result.find("precision") == std::string::npos) {
            result = insertAfterVersion(result, "precision highp float;");
        }
    }

    spdlog::debug("Preprocessed shader for OpenGL ES");
    return result;
}

std::string ShaderPreprocessor::processOpenGL(const std::string& source, uint32_t shaderType) {
    return source;
}

std::string ShaderPreprocessor::replaceVersion(const std::string& source,
                                              const std::string& fromVersion,
                                              const std::string& toVersion) {
    std::string result = source;
    size_t pos = result.find(fromVersion);

    if (pos != std::string::npos) {
        result.replace(pos, fromVersion.length(), toVersion);
        spdlog::debug("Replaced '{}' with '{}'", fromVersion, toVersion);
    }

    return result;
}

std::string ShaderPreprocessor::insertAfterVersion(const std::string& source,
                                                  const std::string& insertion) {
    std::string result = source;

    size_t versionPos = result.find("#version");
    if (versionPos == std::string::npos) {
        spdlog::warn("No #version directive found in shader");
        return result;
    }

    size_t lineEnd = result.find('\n', versionPos);
    if (lineEnd == std::string::npos) {
        lineEnd = result.length();
    }

    result.insert(lineEnd + 1, "\n" + insertion + "\n");
    spdlog::debug("Inserted '{}' after version directive", insertion);

    return result;
}

} // namespace kaisei::backend
