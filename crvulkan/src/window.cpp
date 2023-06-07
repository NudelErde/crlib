//
// Created by nudelerde on 05.06.23.
//

#include "window.h"
#include <GLFW/glfw3.h>

static bool initialized = false;

static void initGlfw() {
    if (!initialized) {
        glfwInit();
        initialized = true;
    }
}

namespace cr::vulkan {
void closeGlfw() {
    if (initialized)
        glfwTerminate();
    initialized = false;
}

std::shared_ptr<Window> createWindow(uint32_t width, uint32_t height) {
    auto window = std::make_shared<Window>();
    window->width = width;
    window->height = height;
    initGlfw();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window->window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), "Vulkan", nullptr,
                                      nullptr);
    return window;
}

Window::~Window() {
    if (window)
        glfwDestroyWindow(window);
}

uint32_t Window::getWidth() const {
    return width;
}

uint32_t Window::getHeight() const {
    return height;
}

void Window::setTitle(const char* title) {
    glfwSetWindowTitle(window, title);
}

void Window::pollEvents() {
    glfwPollEvents();
}
bool Window::shouldClose() const {
    return glfwWindowShouldClose(window);
}
math::cvector<uint32_t, 2> Window::getFramebufferSize() const {
    math::cvector<uint32_t, 2> size;
    glfwGetFramebufferSize(window, reinterpret_cast<int*>(&size[0]), reinterpret_cast<int*>(&size[1]));
    return size;
}
}// namespace cr::vulkan