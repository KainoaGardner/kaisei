#include <memory>
#include <spdlog/spdlog.h>

#include "integration/hyprland/hyprlandPlugin.h"
#include "integration/hyprland/hyprlandRenderer.h"
#include "integration/hyprland/hyprlandServer.h"
#include "core/registry.h"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/SharedDefs.hpp>
#include <hyprland/src/helpers/Monitor.hpp>

namespace kaisei::integration::hyprland {

HyprlandRenderer* g_renderer = nullptr;
HyprlandServer* g_server = nullptr;
HANDLE g_pluginHandle = nullptr;

static std::unique_ptr<core::Registry> g_registry;

static void onRenderPre(PHLMONITOR monitor) {
    if (!g_renderer || !g_renderer->isEnabled()) {
        return;
    }

    if (!monitor) {
        return;
    }

    uint32_t width = monitor->m_pixelSize.x;
    uint32_t height = monitor->m_pixelSize.y;

    // TODO: Get actual input texture and output FBO from Hyprland's render context
    // For now, this is a placeholder showing the structure
    uint32_t inputTexture = 0;  // Placeholder
    uint32_t outputFbo = 0;     // Placeholder - render to currently bound FBO

    g_renderer->render(inputTexture, outputFbo, width, height);
}

static void handleCommand(const std::string& command) {
    spdlog::info("Received command: {}", command);

    if (command.rfind("LOAD:", 0) == 0) {
        std::string presetName = command.substr(5);
        try {
            g_renderer->loadPreset(presetName);
            spdlog::info("Loaded preset: {}", presetName);
        } catch (const std::exception& e) {
            spdlog::error("Failed to load preset '{}': {}", presetName, e.what());
        }
    } else if (command == "TOGGLE") {
        g_renderer->setEnabled(!g_renderer->isEnabled());
        spdlog::info("Toggled effects: {}", g_renderer->isEnabled() ? "ON" : "OFF");
    } else if (command == "STOP") {
        g_renderer->setEnabled(false);
        spdlog::info("Stopped effects");
    } else if (command == "STATUS") {
        spdlog::info(g_renderer->getStatus());
    } else {
        spdlog::warn("Unknown command: {}", command);
    }
}

} // namespace kaisei::integration::hyprland


APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    using namespace kaisei::integration::hyprland;

    g_pluginHandle = handle;

    spdlog::set_level(spdlog::level::info);
    spdlog::info("Kaisei Hyprland plugin initializing...");

    try {
        g_registry = std::make_unique<kaisei::core::Registry>();
        g_renderer = new HyprlandRenderer(*g_registry);

        g_server = new HyprlandServer();
        g_server->setCommandCallback(handleCommand);
        g_server->start("/tmp/kaisei-hyprland.sock");

        static auto renderListener = Event::bus()->m_events.render.pre.listen(
            [](PHLMONITOR monitor) {
                onRenderPre(monitor);
            }
        );

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
