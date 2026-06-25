#include "backend/openGLBackend.h"

#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace kaisei::backend {

OpenGLBackend::OpenGLBackend()
    : quadVAO_(0), quadVBO_(0), initialized_(false) {
}

OpenGLBackend::~OpenGLBackend() {
    if (initialized_) {
        shutdown();
    }
}

bool OpenGLBackend::initialize() {
    if (initialized_) {
        spdlog::warn("OpenGL backend already initialized");
        return true;
    }

    if (!gladLoadGL()) {
        spdlog::error("Failed to initialize glad (OpenGL function loader)");
        return false;
    }

    spdlog::info("OpenGL Version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    spdlog::info("GLSL Version: {}", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    spdlog::info("Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    createFullscreenQuad();

    initialized_ = true;
    spdlog::debug("OpenGL backend initialized");
    return true;
}

void OpenGLBackend::shutdown() {
    if (!initialized_) {
        return;
    }

    destroyFullscreenQuad();

    for (auto& [id, fbo] : framebuffers_) {
        glDeleteFramebuffers(1, &fbo.fbo);
        glDeleteTextures(1, &fbo.colorTexture);
    }
    framebuffers_.clear();

    initialized_ = false;
    spdlog::debug("OpenGL backend shut down");
}

uint32_t OpenGLBackend::compileShader(const std::string& source, uint32_t type) {
    GLenum glType = (type == VERTEX_SHADER) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;

    GLuint shader = glCreateShader(glType);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        spdlog::error("Shader compilation failed: {}", infoLog);
        glDeleteShader(shader);
        throw std::runtime_error("Shader compilation failed: " + std::string(infoLog));
    }

    return shader;
}

uint32_t OpenGLBackend::linkProgram(uint32_t vertexShader, uint32_t fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        spdlog::error("Program linking failed: {}", infoLog);
        glDeleteProgram(program);
        throw std::runtime_error("Program linking failed: " + std::string(infoLog));
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void OpenGLBackend::useProgram(uint32_t program) {
    glUseProgram(program);
}

void OpenGLBackend::deleteProgram(uint32_t program) {
    glDeleteProgram(program);
}

uint32_t OpenGLBackend::createTexture(uint32_t width, uint32_t height, const void* data) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    textureSizes_[texture] = {width, height};

    return texture;
}

uint32_t OpenGLBackend::loadTextureFromFile(const std::string& filePath) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 4); 

    if (!data) {
        spdlog::error("Failed to load texture: {}", filePath);
        throw std::runtime_error("Failed to load texture: " + filePath);
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    textureSizes_[texture] = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    spdlog::debug("Loaded texture from {}: {}x{}", filePath, width, height);

    return texture;
}

void OpenGLBackend::bindTexture(uint32_t texture, uint32_t slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture);
}

void OpenGLBackend::deleteTexture(uint32_t texture) {
    glDeleteTextures(1, &texture);
    textureSizes_.erase(texture);
}

void OpenGLBackend::getTextureSize(uint32_t texture, uint32_t& width, uint32_t& height) {
    auto it = textureSizes_.find(texture);
    if (it != textureSizes_.end()) {
        width = it->second.first;
        height = it->second.second;
    } else {
        width = height = 0;
    }
}

uint32_t OpenGLBackend::createFramebuffer(uint32_t width, uint32_t height) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    GLuint colorTexture = createTexture(width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("Framebuffer is not complete");
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &colorTexture);
        throw std::runtime_error("Framebuffer is not complete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    framebuffers_[fbo] = {fbo, colorTexture, width, height};

    return fbo;
}

void OpenGLBackend::bindFramebuffer(uint32_t framebuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

void OpenGLBackend::unbindFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t OpenGLBackend::getFramebufferTexture(uint32_t framebuffer) {
    auto it = framebuffers_.find(framebuffer);
    if (it != framebuffers_.end()) {
        return it->second.colorTexture;
    }
    return 0;
}

void OpenGLBackend::deleteFramebuffer(uint32_t framebuffer) {
    auto it = framebuffers_.find(framebuffer);
    if (it != framebuffers_.end()) {
        glDeleteFramebuffers(1, &it->second.fbo);
        glDeleteTextures(1, &it->second.colorTexture);
        framebuffers_.erase(it);
    }
}

void OpenGLBackend::setUniformFloat(uint32_t program, const std::string& name, float value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void OpenGLBackend::setUniformInt(uint32_t program, const std::string& name, int value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void OpenGLBackend::setUniformVec2(uint32_t program, const std::string& name, float x, float y) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform2f(location, x, y);
    }
}

void OpenGLBackend::setUniformVec3(uint32_t program, const std::string& name, float x, float y, float z) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

void OpenGLBackend::setUniformVec4(uint32_t program, const std::string& name, float x, float y, float z, float w) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform4f(location, x, y, z, w);
    }
}

void OpenGLBackend::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLBackend::drawFullscreenQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void OpenGLBackend::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    glViewport(x, y, width, height);
}

void OpenGLBackend::createFullscreenQuad() {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);

    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    spdlog::debug("Created fullscreen quad");
}

void OpenGLBackend::destroyFullscreenQuad() {
    if (quadVAO_) {
        glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
    }
    if (quadVBO_) {
        glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }
}

} // namespace kaisei::backend
