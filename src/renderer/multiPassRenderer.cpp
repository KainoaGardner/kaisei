#include "renderer/multiPassRenderer.h"

#include <ctime>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace kaisei::renderer {

namespace {

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} 

MultiPassRenderer::MultiPassRenderer(backend::Backend& backend, core::ModuleLoader& moduleLoader)
    : backend_(backend),
      moduleLoader_(moduleLoader),
      vertexShader_(0),
      passthroughProgram_(0),
      currentPreset_(nullptr),
      startTime_(std::chrono::steady_clock::now()),
      lastFrameTime_(std::chrono::steady_clock::now()),
      frameCount_(0),
      mouseX_(0.0f),
      mouseY_(0.0f),
      mouseLeftButton_(false),
      mouseRightButton_(false),
      randomEngine_(std::random_device{}()),
      randomDist_(0.0f, 1.0f)
    {

    vertexShaderSource_ = readFile("shaders/vertex.vert");
    vertexShader_ = backend_.compileShader(vertexShaderSource_, backend::Backend::VERTEX_SHADER);

    std::string passthroughSource = readFile("shaders/passthrough.frag");
    uint32_t passthroughFrag = backend_.compileShader(passthroughSource, backend::Backend::FRAGMENT_SHADER);
    passthroughProgram_ = backend_.linkProgram(vertexShader_, passthroughFrag);

    spdlog::debug("MultiPassRenderer initialized");
}

MultiPassRenderer::~MultiPassRenderer() {
    cleanupPasses();
    cleanupFramebuffers();

    if (passthroughProgram_) {
        backend_.deleteProgram(passthroughProgram_);
    }
    if (vertexShader_) {
        backend_.deleteProgram(vertexShader_);
    }
}

void MultiPassRenderer::loadPreset(const core::Preset& preset) {
    cleanupPasses();
    cleanupFramebuffers();

    currentPreset_ = &preset;

    for (const auto& module : preset.modules()) {
        compilePass(module.moduleName);
    }

    spdlog::info("Loaded preset '{}' with {} passes", preset.name(), passes_.size());
}

void MultiPassRenderer::compilePass(const std::string& moduleName) {
    const auto* module = moduleLoader_.getModule(moduleName);
    if (!module) {
        spdlog::error("Module '{}' not found", moduleName);
        throw std::runtime_error("Module not found: " + moduleName);
    }

    uint32_t fragShader = backend_.compileShader(
        module->shaderSource(),
        backend::Backend::FRAGMENT_SHADER
    );

    uint32_t program = backend_.linkProgram(vertexShader_, fragShader);

    passes_.push_back({program, moduleName});

    spdlog::debug("Compiled pass for module '{}'", moduleName);
}

uint32_t MultiPassRenderer::render(uint32_t inputTexture, uint32_t width, uint32_t height, uint32_t outputFbo) {
    auto now = std::chrono::steady_clock::now();

    if (passes_.empty()) {
        backend_.bindFramebuffer(outputFbo);
        backend_.setViewport(0, 0, width, height);
        backend_.clear(0.0f, 0.0f, 0.0f, 1.0f);

        backend_.useProgram(passthroughProgram_);
        setAutomaticUniforms(passthroughProgram_, inputTexture, width, height, now);

        backend_.bindTexture(inputTexture, 0);
        backend_.setUniformInt(passthroughProgram_, "u_inputTexture", 0);
        backend_.drawFullscreenQuad();

        frameCount_++;
        lastFrameTime_ = now;
        return inputTexture;
    }

    while (framebuffers_.size() < 2) {
        framebuffers_.push_back(backend_.createFramebuffer(width, height));
    }

    uint32_t currentInput = inputTexture;
    uint32_t currentFBO = 0;

    for (size_t i = 0; i < passes_.size(); ++i) {
        const auto& pass = passes_[i];

        if (i == passes_.size() - 1) {
            currentFBO = outputFbo;
        } else {
            currentFBO = framebuffers_[i % 2];
        }

        backend_.bindFramebuffer(currentFBO);

        backend_.setViewport(0, 0, width, height);
        backend_.clear(0.0f, 0.0f, 0.0f, 1.0f);

        backend_.useProgram(pass.program);
        setAutomaticUniforms(pass.program, currentInput, width, height, now);

        backend_.bindTexture(currentInput, 0);
        backend_.setUniformInt(pass.program, "u_inputTexture", 0);

        const auto* module = moduleLoader_.getModule(pass.moduleName);
        if (module) {
            for (const auto& uniform : module->metadata().uniforms) {
                if (uniform.defaultValue) {
                    try {
                        float fvalue = std::stof(*uniform.defaultValue);
                        backend_.setUniformFloat(pass.program, uniform.name, fvalue);
                    } catch (...) {
                        try {
                            int ivalue = std::stoi(*uniform.defaultValue);
                            backend_.setUniformInt(pass.program, uniform.name, ivalue);
                        } catch (...) {
                            spdlog::warn("Failed to parse default uniform '{}' value '{}'",
                                       uniform.name, *uniform.defaultValue);
                        }
                    }
                }
            }

            if (currentPreset_) {
                for (const auto& presetModule : currentPreset_->modules()) {
                    if (presetModule.moduleName == pass.moduleName) {
                        for (const auto& [name, value] : presetModule.uniformOverrides) {
                            try {
                                float fvalue = std::stof(value);
                                backend_.setUniformFloat(pass.program, name, fvalue);
                            } catch (...) {
                                try {
                                    int ivalue = std::stoi(value);
                                    backend_.setUniformInt(pass.program, name, ivalue);
                                } catch (...) {
                                    spdlog::warn("Failed to parse uniform '{}' value '{}'", name, value);
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }

        backend_.drawFullscreenQuad();

        if (i < passes_.size() - 1) {
            currentInput = backend_.getFramebufferTexture(currentFBO);
        }
    }

    frameCount_++;
    lastFrameTime_ = now;

    if (passes_.size() == 1) {
        return backend_.getFramebufferTexture(framebuffers_[0]);
    } else {
        return backend_.getFramebufferTexture(framebuffers_[(passes_.size() - 2) % 2]);
    }
}

void MultiPassRenderer::reload() {
    if (!currentPreset_) {
        return;
    }

    spdlog::info("Reloading preset '{}'", currentPreset_->name());
    loadPreset(*currentPreset_);
}

void MultiPassRenderer::cleanupPasses() {
    for (auto& pass : passes_) {
        backend_.deleteProgram(pass.program);
    }
    passes_.clear();
}

void MultiPassRenderer::cleanupFramebuffers() {
    for (auto fbo : framebuffers_) {
        backend_.deleteFramebuffer(fbo);
    }
    framebuffers_.clear();
}

void MultiPassRenderer::setAutomaticUniforms(uint32_t program, uint32_t inputTexture, uint32_t width, uint32_t height,
                                             const std::chrono::steady_clock::time_point& currentFrameTime) {
    float elapsed = std::chrono::duration<float>(currentFrameTime - startTime_).count();
    float deltaTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime_).count();

    backend_.setUniformFloat(program, "u_time", elapsed);
    backend_.setUniformFloat(program, "u_deltaTime", deltaTime);
    backend_.setUniformVec2(program, "u_resolution", static_cast<float>(width), static_cast<float>(height));
    backend_.setUniformVec2(program, "u_texelSize", 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height));
    backend_.setUniformInt(program, "u_frame", frameCount_);
    backend_.setUniformFloat(program, "u_random", randomDist_(randomEngine_));

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&time_t_now);
    float year = static_cast<float>(local_time->tm_year + 1900);
    float month = static_cast<float>(local_time->tm_mon + 1);
    float day = static_cast<float>(local_time->tm_mday);
    float seconds = static_cast<float>(local_time->tm_hour * 3600 + local_time->tm_min * 60 + local_time->tm_sec);
    backend_.setUniformVec4(program, "u_date", year, month, day, seconds);

    uint32_t inputWidth, inputHeight;
    backend_.getTextureSize(inputTexture, inputWidth, inputHeight);
    backend_.setUniformVec2(program, "u_inputSize", static_cast<float>(inputWidth), static_cast<float>(inputHeight));

    backend_.setUniformVec2(program, "u_mouse", mouseX_, mouseY_);
    backend_.setUniformVec4(program, "u_mouseButtons",
        mouseLeftButton_ ? 1.0f : 0.0f,
        mouseRightButton_ ? 1.0f : 0.0f,
        0.0f, 0.0f);
}

} // namespace kaisei::renderer
