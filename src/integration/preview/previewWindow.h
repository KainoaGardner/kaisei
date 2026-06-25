#pragma once

#include <functional>
#include <string>

struct GLFWwindow;

namespace kaisei::integration::preview {

class PreviewWindow {
public:
    PreviewWindow(int width, int height, const std::string& title);
    ~PreviewWindow();

    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();

    void getFramebufferSize(int& width, int& height) const;
    GLFWwindow* getHandle() const { return window_; }

    void setKeyCallback(std::function<void(int key, int action)> callback);
    void setMousePosCallback(std::function<void(double x, double y)> callback);
    void setMouseButtonCallback(std::function<void(int button, int action)> callback);

private:
    static void keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mousePosCallbackStatic(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods);

    GLFWwindow* window_;
    std::function<void(int key, int action)> keyCallback_;
    std::function<void(double x, double y)> mousePosCallback_;
    std::function<void(int button, int action)> mouseButtonCallback_;
};

} // namespace kaisei::integration::preview
