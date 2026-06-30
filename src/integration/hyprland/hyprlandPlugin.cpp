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
static uint64_t g_frameCount = 0;

static void onRenderStage(eRenderStage stage) {
    if (stage != RENDER_LAST_MOMENT) {
        return;
    }

    if (!g_renderer || !g_renderer->isEnabled()) {
        return;
    }

    if (!g_pHyprRenderer) {
        spdlog::warn("g_pHyprRenderer is null");
        return;
    }

    const auto& renderData = g_pHyprRenderer->renderData();

    auto monitor = renderData.pMonitor.lock();
    if (!monitor) {
        spdlog::warn("Monitor is null in render data");
        return;
    }

    uint32_t width = monitor->m_pixelSize.x;
    uint32_t height = monitor->m_pixelSize.y;

    g_frameCount++;

    uint32_t inputTexture = 0;
    uint32_t outputFbo = 0;

    if (renderData.mainFB) {
        auto tex = renderData.mainFB->getTexture();
        if (tex) {
            inputTexture = tex->m_texID;
        }
    }

    SP<Render::IFramebuffer> targetFB = renderData.outFB;
    if (!targetFB) {
        spdlog::warn("outFB is null, falling back to currentFB");
        targetFB = renderData.currentFB;
    }

    if (targetFB) {
        auto* glFB = dynamic_cast<Render::GL::CGLFramebuffer*>(targetFB.get());
        if (glFB) {
            outputFbo = glFB->getFBID();

            static bool logged_switch = false;
            if (!logged_switch) {
                uint32_t currentFboId = 0;
                if (renderData.currentFB) {
                    auto* currentGlFB = dynamic_cast<Render::GL::CGLFramebuffer*>(renderData.currentFB.get());
                    if (currentGlFB) currentFboId = currentGlFB->getFBID();
                }
                spdlog::info("Switched to rendering to outFB: {} (currentFB was: {})", outputFbo, currentFboId);
                logged_switch = true;
            }
        }
    }

    if (inputTexture > 0 && outputFbo > 0) {
        g_renderer->render(inputTexture, outputFbo, width, height);
    }
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

        spdlog::debug("Registering render stage listener...");
        static auto renderListener = Event::bus()->m_events.render.stage.listen(
            [](eRenderStage stage) {
                onRenderStage(stage);
            }
        );
        spdlog::debug("Render stage listener registered");

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
