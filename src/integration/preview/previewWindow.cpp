#include "integration/preview/previewWindow.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace kaisei::integration::preview {

static void glfwErrorCallback(int error, const char* description) {
    spdlog::error("GLFW Error {}: {}", error, description);
}

PreviewWindow::PreviewWindow(int width, int height, const std::string& title)
    : window_(nullptr) {

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, keyCallbackStatic);

    spdlog::info("Created preview window: {}x{}", width, height);
}

PreviewWindow::~PreviewWindow() {
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool PreviewWindow::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void PreviewWindow::pollEvents() {
    glfwPollEvents();
}

void PreviewWindow::swapBuffers() {
    glfwSwapBuffers(window_);
}

void PreviewWindow::getFramebufferSize(int& width, int& height) const {
    glfwGetFramebufferSize(window_, &width, &height);
}

void PreviewWindow::setKeyCallback(std::function<void(int key, int action)> callback) {
    keyCallback_ = callback;
}

void PreviewWindow::keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* previewWindow = static_cast<PreviewWindow*>(glfwGetWindowUserPointer(window));
    if (previewWindow && previewWindow->keyCallback_) {
        previewWindow->keyCallback_(key, action);
    }
}

} // namespace kaisei::integration::preview
