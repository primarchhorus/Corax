#pragma once

#include "vulkan_common.h"

#include <cstddef>
#include <functional>

namespace Vulkan {

class Instance;
using ResizeCallback = std::function<void(int width, int height)>;

struct Window {

    Window();
    Window(size_t window_width, size_t window_height, const std::string& window_title);
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;
    ~Window();

    void destroy();
    void setResizeCallback(ResizeCallback callback);
    void getWindowFramebufferSize(int& width, int& height);
    int closeCheck();

    static void windowResizeCallback(GLFWwindow* window, int width, int height) {
        Window* active_window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        active_window->window_resized = true;
        active_window->width = width;
        active_window->width = height;

        if (active_window->resize_callback) {
            active_window->resize_callback(width, height);
        }
    }

    GLFWwindow* handle{nullptr};
    size_t width{0};
    size_t height{0};
    bool window_resized{false};
    ResizeCallback resize_callback;
};
}