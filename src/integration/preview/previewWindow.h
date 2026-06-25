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

private:
    static void keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* window_;
    std::function<void(int key, int action)> keyCallback_;
};

} // namespace kaisei::integration::preview
