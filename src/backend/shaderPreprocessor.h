#pragma once

#include <cstdint>
#include <string>

namespace kaisei::backend {

enum class BackendType {
    OpenGL,
    OpenGLES,
    Vulkan  // Future
};

enum class ShaderStage {
    Vertex,
    Fragment
};

class ShaderPreprocessor {
public:
    static std::string process(const std::string& source,
                              BackendType backend,
                              uint32_t shaderType);

private:
    static std::string processOpenGLES(const std::string& source, uint32_t shaderType);
    static std::string processOpenGL(const std::string& source, uint32_t shaderType);

    static std::string replaceVersion(const std::string& source,
                                      const std::string& fromVersion,
                                      const std::string& toVersion);
    static std::string insertAfterVersion(const std::string& source,
                                         const std::string& insertion);
};

} // namespace kaisei::backend
