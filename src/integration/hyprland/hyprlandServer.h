#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace kaisei::integration::hyprland {

class HyprlandServer {
public:
    HyprlandServer();
    ~HyprlandServer();

    void start(const std::string& socketPath);
    void stop();

    void setCommandCallback(std::function<std::string(const std::string&)> callback);

private:
    void acceptLoop();
    void handleClient(int clientFd);

    int serverSocket_;
    std::string socketPath_;
    std::thread acceptThread_;
    std::atomic<bool> running_;
    std::function<std::string(const std::string&)> commandCallback_;
};

} // namespace kaisei::integration::hyprland
