#include "integration/hyprland/hyprlandServer.h"

#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace kaisei::integration::hyprland {

HyprlandServer::HyprlandServer()
    : serverSocket_(-1), running_(false) {
}

HyprlandServer::~HyprlandServer() {
    stop();
}

void HyprlandServer::start(const std::string& socketPath) {
    if (running_) {
        spdlog::warn("Server already running");
        return;
    }

    socketPath_ = socketPath;

    unlink(socketPath_.c_str());

    serverSocket_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        spdlog::error("Failed to create socket: {}", strerror(errno));
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath_.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(serverSocket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        spdlog::error("Failed to bind socket: {}", strerror(errno));
        close(serverSocket_);
        serverSocket_ = -1;
        return;
    }

    if (listen(serverSocket_, 5) < 0) {
        spdlog::error("Failed to listen on socket: {}", strerror(errno));
        close(serverSocket_);
        serverSocket_ = -1;
        return;
    }

    running_ = true;
    acceptThread_ = std::thread(&HyprlandServer::acceptLoop, this);

    spdlog::info("Hyprland server started on {}", socketPath_);
}

void HyprlandServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }

    unlink(socketPath_.c_str());
    spdlog::info("Hyprland server stopped");
}

void HyprlandServer::setCommandCallback(std::function<void(const std::string&)> callback) {
    commandCallback_ = callback;
}

void HyprlandServer::acceptLoop() {
    while (running_) {
        int clientFd = accept(serverSocket_, nullptr, nullptr);
        if (clientFd < 0) {
            if (running_) {
                spdlog::error("Accept failed: {}", strerror(errno));
            }
            continue;
        }

        handleClient(clientFd);
        close(clientFd);
    }
}

void HyprlandServer::handleClient(int clientFd) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        return;
    }

    std::string command(buffer, bytesRead);
    spdlog::debug("Received command: {}", command);

    if (commandCallback_) {
        try {
            commandCallback_(command);
            const char* response = "OK\n";
            write(clientFd, response, strlen(response));
        } catch (const std::exception& e) {
            std::string errorMsg = std::string("ERROR:") + e.what() + "\n";
            write(clientFd, errorMsg.c_str(), errorMsg.length());
            spdlog::error("Command failed: {}", e.what());
        }
    } else {
        const char* response = "ERROR:No command handler registered\n";
        write(clientFd, response, strlen(response));
    }
}

} // namespace kaisei::integration::hyprland
