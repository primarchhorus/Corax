#pragma once

#include "vulkan_common.h"

#include <cstddef>
#include <functional>

namespace Vulkan {

struct Instance;
using ResizeCallback = std::function<void(int width, int height)>;

struct Window {

    Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;
    ~Window();

    void create(uint32_t window_width, uint32_t window_height, const std::string& window_title);
    void destroy();
    void setResizeCallback(ResizeCallback callback);
    void getWindowFramebufferSize(int& width, int& height) const;
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