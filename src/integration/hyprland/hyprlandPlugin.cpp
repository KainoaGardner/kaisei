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

namespace kaisei::integration::hyprland {

HyprlandRenderer* g_renderer = nullptr;
HyprlandServer* g_server = nullptr;
HANDLE g_pluginHandle = nullptr;

static std::unique_ptr<core::Registry> g_registry;

// Hook for CHyprOpenGLImpl::end()
typedef void (*end_t)(Render::GL::CHyprOpenGLImpl*);
static end_t g_pOriginalEnd = nullptr;

static void hook_end(Render::GL::CHyprOpenGLImpl* thisptr) {
    g_pOriginalEnd(thisptr);

    if (!g_renderer || !g_renderer->isEnabled()) {
        return;
    }

    auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
    if (!monitor) {
        return;
    }

    const auto& renderData = g_pHyprRenderer->m_renderData;
    if (!renderData.currentFB) {
        return;
    }

    auto tex = renderData.currentFB->getTexture();
    if (!tex) {
        return;
    }

    auto* glFB = dynamic_cast<Render::GL::CGLFramebuffer*>(renderData.currentFB.get());
    if (!glFB) {
        return;
    }

    uint32_t inputTexture = tex->m_texID;
    uint32_t outputFbo = glFB->getFBID();
    uint32_t width = monitor->m_pixelSize.x;
    uint32_t height = monitor->m_pixelSize.y;

    g_renderer->render(inputTexture, outputFbo, width, height);
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

        spdlog::debug("Hooking CHyprOpenGLImpl::end()...");
        auto endFunctions = HyprlandAPI::findFunctionsByName(handle, "end");
        void* endAddr = nullptr;

        for (const auto& func : endFunctions) {
            if (func.demangled.find("CHyprOpenGLImpl::end()") != std::string::npos) {
                endAddr = func.address;
                spdlog::info("Found CHyprOpenGLImpl::end() at {}", endAddr);
                break;
            }
        }

        if (!endAddr) {
            spdlog::error("Could not find CHyprOpenGLImpl::end()");
            throw std::runtime_error("Failed to find end()");
        }

        static auto endHook = HyprlandAPI::createFunctionHook(handle, endAddr, (void*)&hook_end);
        if (!endHook->hook()) {
            spdlog::error("Failed to hook CHyprOpenGLImpl::end()");
            throw std::runtime_error("Failed to hook end()");
        }
        g_pOriginalEnd = (end_t)endHook->m_original;
        spdlog::debug("Successfully hooked end()");

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
