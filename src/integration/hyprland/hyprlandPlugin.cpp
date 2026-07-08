#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <GLES3/gl32.h>
#include <spdlog/spdlog.h>

#include "integration/hyprland/hyprlandPlugin.h"
#include "integration/hyprland/hyprlandRenderer.h"
#include "integration/hyprland/hyprlandServer.h"
#include "core/registry.h"
#include "utils/format.h"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/SharedDefs.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Framebuffer.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <hyprland/src/render/gl/GLFramebuffer.hpp>
#include <hyprland/src/render/pass/Pass.hpp>

namespace kaisei::integration::hyprland {

HyprlandRenderer* g_renderer = nullptr;
HyprlandServer* g_server = nullptr;
HANDLE g_pluginHandle = nullptr;

static std::unique_ptr<core::Registry> g_registry;
static CFunctionHook* g_renderHook = nullptr;
static std::atomic<bool> g_shuttingDown{false};

typedef CRegion (*renderPass_t)(Render::CRenderPass*, const CRegion&);
static renderPass_t g_pOriginalRenderPass = nullptr;

static void damageAllMonitors() {
    for (auto& m : g_pCompositor->m_monitors) {
        g_pHyprRenderer->damageMonitor(m);
    }
}

static CRegion hook_renderPass(Render::CRenderPass* thisptr, const CRegion& damage) {
    if (g_shuttingDown.load()) {
        return g_pOriginalRenderPass(thisptr, damage);
    }

    const auto& renderData = g_pHyprRenderer->m_renderData;
    bool shouldProcess = g_renderer && g_renderer->isEnabled() &&
                        renderData.currentFB && renderData.mainFB &&
                        renderData.currentFB == renderData.mainFB;

    CRegion useDamage = damage;
    if (shouldProcess && !damage.empty()) {
        auto monitor = renderData.pMonitor.lock();
        if (monitor) {
            useDamage.clear();
            useDamage.add(CBox{0.0, 0.0, static_cast<double>(monitor->m_pixelSize.x),
                              static_cast<double>(monitor->m_pixelSize.y)});
        }
    }

    CRegion result = g_pOriginalRenderPass(thisptr, useDamage);

    if (!shouldProcess) {
        return result;
    }

    auto monitor = renderData.pMonitor.lock();
    if (!monitor) {
        return result;
    }

    auto tex = renderData.currentFB->getTexture();
    if (!tex) {
        return result;
    }

    auto* glFB = dynamic_cast<Render::GL::CGLFramebuffer*>(renderData.currentFB.get());
    if (!glFB) {
        return result;
    }

    GLint oldProgram, oldFBO, oldVAO;
    GLint oldViewport[4];
    GLboolean oldBlend, oldDepthTest, oldScissorTest;
    glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVAO);
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    oldBlend = glIsEnabled(GL_BLEND);
    oldDepthTest = glIsEnabled(GL_DEPTH_TEST);
    oldScissorTest = glIsEnabled(GL_SCISSOR_TEST);

    g_renderer->render(tex->m_texID, glFB->getFBID(),
                      monitor->m_pixelSize.x, monitor->m_pixelSize.y);

    glUseProgram(oldProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
    glBindVertexArray(oldVAO);
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    if (oldBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (oldDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (oldScissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);

    CRegion fullDamage;
    fullDamage.add(CBox{0.0, 0.0, static_cast<double>(monitor->m_pixelSize.x),
                        static_cast<double>(monitor->m_pixelSize.y)});
    return fullDamage;
}

enum class CommandType {
    On,
    Off,
    Toggle,
    Status,
    Unknown
};

static CommandType parseCommand(const std::string& command, std::string& arg) {
    if (command == "ON") {
        return CommandType::On;
    } else if (command == "OFF") {
        return CommandType::Off;
    } else if (command == "TOGGLE") {
        return CommandType::Toggle;
    } else if (command == "STATUS") {
        return CommandType::Status;
    }
    return CommandType::Unknown;
}

static std::string handleCommand(const std::string& command) {
    spdlog::info("Received command: {}", command);

    if (g_shuttingDown.load() || !g_renderer) {
        return "ERROR:Plugin is shutting down";
    }

    std::string arg;
    CommandType cmdType = parseCommand(command, arg);

    switch (cmdType) {
        case CommandType::On:
            try {
                g_renderer->loadCurrentPreset();
                g_renderer->setEnabled(true);
                spdlog::info("Effects enabled");
                damageAllMonitors();
                return utils::bold("Effects enabled with preset '" + g_renderer->getCurrentPreset() + "'");
            } catch (const std::exception& e) {
                spdlog::error("Failed to enable effects: {}", e.what());
                return "ERROR:" + std::string(e.what());
            }

        case CommandType::Off:
            g_renderer->setEnabled(false);
            spdlog::info("Effects disabled");
            damageAllMonitors();
            return utils::bold("Effects disabled");

        case CommandType::Toggle:
            try {
                bool newState = !g_renderer->isEnabled();
                if (newState) {
                    g_renderer->loadCurrentPreset();
                }
                g_renderer->setEnabled(newState);
                spdlog::info("Toggled effects: {}", newState ? "ON" : "OFF");
                damageAllMonitors();
                return utils::bold(newState ? ("Effects enabled with preset '" + g_renderer->getCurrentPreset() + "'") : "Effects disabled");
            } catch (const std::exception& e) {
                spdlog::error("Failed to toggle effects: {}", e.what());
                return "ERROR:" + std::string(e.what());
            }

        case CommandType::Status:
            return g_renderer->getStatus();

        case CommandType::Unknown:
            spdlog::warn("Unknown command: {}", command);
            return "ERROR:Unknown command '" + command + "'";
    }

    return "OK";
}

} // namespace kaisei::integration::hyprland


APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    using namespace kaisei::integration::hyprland;

    g_pluginHandle = handle;
    g_shuttingDown.store(false); 

    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Kaisei Hyprland plugin initializing...");

    try {
        spdlog::debug("Creating registry...");
        g_registry = std::make_unique<kaisei::core::Registry>();
        g_registry->initialize();
        spdlog::debug("Registry initialized");

        spdlog::debug("Creating renderer...");
        g_renderer = new HyprlandRenderer(*g_registry);
        spdlog::debug("Renderer created");

        spdlog::debug("Starting command server...");
        g_server = new HyprlandServer();
        g_server->setCommandCallback(handleCommand);
        g_server->start("/tmp/kaisei-hyprland.sock");
        spdlog::debug("Command server started");

        spdlog::debug("Hooking CRenderPass::render()...");
        auto renderFunctions = HyprlandAPI::findFunctionsByName(handle, "render");
        void* renderAddr = nullptr;

        spdlog::debug("Found {} functions with 'render' in name", renderFunctions.size());
        for (const auto& func : renderFunctions) {
            if (func.demangled.find("CRenderPass") != std::string::npos) {
                spdlog::debug("  - {}", func.demangled);
            }
            if (func.demangled.find("CRenderPass::render") != std::string::npos &&
                func.demangled.find("renderDebugData") == std::string::npos) {
                renderAddr = func.address;
                spdlog::info("Found CRenderPass::render() at {}", renderAddr);
                break;
            }
        }

        if (!renderAddr) {
            spdlog::error("Could not find CRenderPass::render()");
            throw std::runtime_error("Failed to find CRenderPass::render()");
        }

        g_renderHook = HyprlandAPI::createFunctionHook(handle, renderAddr, (void*)&hook_renderPass);
        if (!g_renderHook->hook()) {
            spdlog::error("Failed to hook CRenderPass::render()");
            throw std::runtime_error("Failed to hook CRenderPass::render()");
        }
        g_pOriginalRenderPass = (renderPass_t)g_renderHook->m_original;
        spdlog::debug("Successfully hooked CRenderPass::render()");

        spdlog::info("Kaisei Hyprland plugin initialized successfully");

    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize Kaisei plugin: {}", e.what());

        delete g_renderer;
        delete g_server;
        g_renderer = nullptr;
        g_server = nullptr;

        return {"kaisei", "Shader effects compositor (FAILED)", "kaisei contributors", "1.0.0"};
    }

    return {"kaisei", "Shader effects compositor", "kaisei contributors", "1.0.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    using namespace kaisei::integration::hyprland;

    spdlog::info("Kaisei Hyprland plugin shutting down...");

    spdlog::debug("Setting shutdown flag...");
    g_shuttingDown.store(true);

    if (g_renderer) {
        spdlog::debug("Disabling renderer...");
        g_renderer->setEnabled(false);
    }

    if (g_renderHook) {
        spdlog::debug("Unhooking CRenderPass::render()...");
        g_renderHook->unhook();
        g_renderHook = nullptr;
        spdlog::debug("Unhooked successfully");
    }

    spdlog::debug("Waiting for in-flight renders...");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (g_server) {
        spdlog::debug("Stopping command server...");
        g_server->stop();
        delete g_server;
        g_server = nullptr;
    }

    if (g_renderer) {
        spdlog::debug("Deleting renderer...");
        delete g_renderer;
        g_renderer = nullptr;
    }

    spdlog::debug("Resetting registry...");
    g_registry.reset();

    spdlog::info("Kaisei Hyprland plugin shut down");
}
