#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

namespace kaisei::integration::hyprland {

class HyprlandRenderer;
class HyprlandServer;

extern HyprlandRenderer* g_renderer;
extern HyprlandServer* g_server;
extern HANDLE g_pluginHandle;

} // namespace kaisei::integration::hyprland
