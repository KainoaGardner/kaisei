#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command>\n";
        std::cerr << "Commands:\n";
        std::cerr << "  load <preset>  - Load a preset\n";
        std::cerr << "  toggle         - Toggle effects on/off\n";
        std::cerr << "  stop           - Stop effects\n";
        std::cerr << "  status         - Get status\n";
        return 1;
    }

    std::string command;
    if (std::string(argv[1]) == "load" && argc >= 3) {
        command = "LOAD:" + std::string(argv[2]);
    } else if (std::string(argv[1]) == "toggle") {
        command = "TOGGLE";
    } else if (std::string(argv[1]) == "stop") {
        command = "STOP";
    } else if (std::string(argv[1]) == "status") {
        command = "STATUS";
    } else {
        std::cerr << "Unknown command: " << argv[1] << "\n";
        return 1;
    }

    const char* socketPath = "/tmp/kaisei-hyprland.sock";

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << "\n";
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to Kaisei plugin. Is Hyprland running with the plugin loaded?\n";
        std::cerr << "Error: " << strerror(errno) << "\n";
        close(sockfd);
        return 1;
    }

    // Send command
    if (write(sockfd, command.c_str(), command.length()) < 0) {
        std::cerr << "Failed to send command: " << strerror(errno) << "\n";
        close(sockfd);
        return 1;
    }

    // Read response
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytesRead = read(sockfd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        std::cout << buffer;
    }

    close(sockfd);
    return 0;
}
