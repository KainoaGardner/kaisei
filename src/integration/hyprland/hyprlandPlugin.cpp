#include <memory>
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

typedef CRegion (*renderPass_t)(Render::CRenderPass*, const CRegion&);
static renderPass_t g_pOriginalRenderPass = nullptr;

static CRegion hook_renderPass(Render::CRenderPass* thisptr, const CRegion& damage) {
    bool shouldProcess = false;
    if (g_renderer && g_renderer->isEnabled()) {
        const auto& renderData = g_pHyprRenderer->m_renderData;
        if (renderData.currentFB && renderData.mainFB && renderData.currentFB == renderData.mainFB) {
            shouldProcess = true;
        }
    }

    // If we're going to apply shader AND there's damage, force full damage so entire FB is rendered
    CRegion useDamage = damage;
    if (shouldProcess && !damage.empty()) {
        auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
        if (monitor) {
            useDamage.clear();
            useDamage.add(CBox{0.0, 0.0, static_cast<double>(monitor->m_pixelSize.x), static_cast<double>(monitor->m_pixelSize.y)});
        }
    }

    CRegion result = g_pOriginalRenderPass(thisptr, useDamage);

    if (!shouldProcess) {
        return result;
    }

    auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
    if (!monitor) {
        return result;
    }

    const auto& renderData = g_pHyprRenderer->m_renderData;
    auto tex = renderData.currentFB->getTexture();
    if (!tex) {
        return result;
    }

    auto* glFB = dynamic_cast<Render::GL::CGLFramebuffer*>(renderData.currentFB.get());
    if (!glFB) {
        return result;
    }

    uint32_t inputTexture = tex->m_texID;
    uint32_t outputFbo = glFB->getFBID();
    uint32_t width = monitor->m_pixelSize.x;
    uint32_t height = monitor->m_pixelSize.y;

    g_renderer->render(inputTexture, outputFbo, width, height);

    // Shader processing affects the entire screen, so return full damage
    CRegion fullDamage;
    fullDamage.add(CBox{0.0, 0.0, static_cast<double>(width), static_cast<double>(height)});
    return fullDamage;
}

enum class CommandType {
    Load,
    On,
    Off,
    Toggle,
    Status,
    Unknown
};

static CommandType parseCommand(const std::string& command, std::string& arg) {
    if (command.starts_with("LOAD:")) {
        arg = command.substr(5);
        return CommandType::Load;
    } else if (command == "ON") {
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

    std::string arg;
    CommandType cmdType = parseCommand(command, arg);

    switch (cmdType) {
        case CommandType::Load:
            try {
                g_renderer->loadPreset(arg);
                spdlog::info("Loaded preset: {}", arg);
                return utils::bold("Loaded preset '" + arg + "'");
            } catch (const std::exception& e) {
                spdlog::error("Failed to load preset '{}': {}", arg, e.what());
                throw;
            }

        case CommandType::On:
            g_renderer->setEnabled(true);
            spdlog::info("Effects enabled");
            return utils::bold("Effects enabled");

        case CommandType::Off:
            g_renderer->setEnabled(false);
            spdlog::info("Effects disabled");
            return utils::bold("Effects disabled");

        case CommandType::Toggle:
            g_renderer->setEnabled(!g_renderer->isEnabled());
            spdlog::info("Toggled effects: {}", g_renderer->isEnabled() ? "ON" : "OFF");
            return utils::bold(g_renderer->isEnabled() ? "Effects enabled" : "Effects disabled");

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

        static auto renderHook = HyprlandAPI::createFunctionHook(handle, renderAddr, (void*)&hook_renderPass);
        if (!renderHook->hook()) {
            spdlog::error("Failed to hook CRenderPass::render()");
            throw std::runtime_error("Failed to hook CRenderPass::render()");
        }
        g_pOriginalRenderPass = (renderPass_t)renderHook->m_original;
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

    if (g_server) {
        g_server->stop();
        delete g_server;
        g_server = nullptr;
    }

    if (g_renderer) {
        delete g_renderer;
        g_renderer = nullptr;
    }

    g_registry.reset();

    spdlog::info("Kaisei Hyprland plugin shut down");
}
