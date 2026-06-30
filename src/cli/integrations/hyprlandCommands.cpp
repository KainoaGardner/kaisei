#include "cli/integrations/hyprlandCommands.h"

#include <iostream>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace kaisei::cli {

namespace {
    constexpr const char* SOCKET_PATH = "/tmp/kaisei-hyprland.sock";
}

void HyprlandCommands::setup(CLI::App* app, core::Registry& registry) {
    // hyprland load <preset>
    auto* load = app->add_subcommand("load", "Load a preset and enable effects");
    static std::string loadPresetName;
    load->add_option("preset", loadPresetName, "Preset name")->required();
    load->callback([&]() { HyprlandCommands::load(registry, loadPresetName); });

    // hyprland on
    auto* on = app->add_subcommand("on", "Enable effects");
    on->callback([]() { HyprlandCommands::on(); });

    // hyprland off
    auto* off = app->add_subcommand("off", "Disable effects");
    off->callback([]() { HyprlandCommands::off(); });

    // hyprland toggle
    auto* toggle = app->add_subcommand("toggle", "Toggle effects on/off");
    toggle->callback([]() { HyprlandCommands::toggle(); });

    // hyprland status
    auto* status = app->add_subcommand("status", "Check Hyprland integration status");
    status->callback([]() { HyprlandCommands::status(); });
}

void HyprlandCommands::load(core::Registry& registry, const std::string& presetName) {
    if (!registry.presets().hasPreset(presetName)) {
        spdlog::error("Preset '{}' not found", presetName);
        return;
    }

    try {
        std::string response = sendCommand("LOAD:" + presetName);
        std::cout << response << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to load preset: {}", e.what());
    }
}

void HyprlandCommands::on() {
    try {
        std::string response = sendCommand("ON");
        std::cout << response << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to enable: {}", e.what());
    }
}

void HyprlandCommands::off() {
    try {
        std::string response = sendCommand("OFF");
        std::cout << response << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to disable: {}", e.what());
    }
}

void HyprlandCommands::toggle() {
    try {
        std::string response = sendCommand("TOGGLE");
        std::cout << response << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to toggle: {}", e.what());
    }
}

void HyprlandCommands::status() {
    try {
        std::string response = sendCommand("STATUS");
        std::cout << response << "\n";
    } catch (const std::exception& e) {
        spdlog::error("Failed to get status: {}", e.what());
    }
}

std::string HyprlandCommands::sendCommand(const std::string& command) {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        throw std::runtime_error("Failed to connect to Hyprland plugin. Is the plugin loaded?");
    }

    if (write(sockfd, command.c_str(), command.length()) < 0) {
        close(sockfd);
        throw std::runtime_error("Failed to send command: " + std::string(strerror(errno)));
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytesRead = read(sockfd, buffer, sizeof(buffer) - 1);

    close(sockfd);

    if (bytesRead <= 0) {
        throw std::runtime_error("Failed to read response");
    }

    std::string response(buffer, bytesRead);

    if (response.rfind("ERROR:", 0) == 0) {
        throw std::runtime_error(response.substr(6));
    }

    if (!response.empty() && response.back() == '\n') {
        response.pop_back();
    }

    return response;
}

} // namespace kaisei::cli
