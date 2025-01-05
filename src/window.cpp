#include "window.h"
#include "instance.h"
#include "vulkan_utils.h"

namespace Vulkan {
Window::Window() {}

Window::Window(size_t window_width, size_t window_height, const std::string& window_title) :
width(window_width),
height(window_height)
{
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    handle = glfwCreateWindow(width, height, window_title.c_str(), nullptr, nullptr);
    if (!handle) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window!");
    }

    // Set up resize callback
    glfwSetWindowUserPointer(handle, this);
    glfwSetFramebufferSizeCallback(handle, windowResizeCallback);
}

Window::Window(Window&& other) noexcept
    : handle(other.handle), width(other.width), height(other.height) {
    other.handle = nullptr;
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other)
    {
        handle = other.handle;
        width = other.width;
        height = other.height;
        other.handle = nullptr;
    }
    return *this;
}

Window::~Window()
{

}

void Window::destroy(const Instance& instance)
{
    vkDestroySurfaceKHR(instance.handle, surface, nullptr);
    if (handle) {
        glfwDestroyWindow(handle);
        if (!glfwGetCurrentContext()) {
            glfwTerminate();
        }
    }
}

void Window::init(const Instance& instance)
{
    initSurface(instance);
}

void Window::setResizeCallback(ResizeCallback callback) {
    resize_callback = std::move(callback);
}

int Window::closeCheck()
{
    return glfwWindowShouldClose(handle);
}

void Window::getWindowFramebufferSize(int& width, int& height)
{
    glfwGetFramebufferSize(handle, &width, &height);
}

void Window::initSurface(const Instance& instance)
{
    vkCheck(glfwCreateWindowSurface(instance.handle, handle, nullptr, &surface));
}

}
