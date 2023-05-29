//
// Created by nudelerde on 19.05.23.
//

#include "window.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <optional>

static bool glfwInitialized = false;

namespace cr::ui {

static constexpr std::optional<MouseButton> glfw_to_cr_mouse_button(int button) {
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return MouseButton::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return MouseButton::Middle;
        case GLFW_MOUSE_BUTTON_4:
            return MouseButton::Button4;
        case GLFW_MOUSE_BUTTON_5:
            return MouseButton::Button5;
        case GLFW_MOUSE_BUTTON_6:
            return MouseButton::Button6;
        case GLFW_MOUSE_BUTTON_7:
            return MouseButton::Button7;
        case GLFW_MOUSE_BUTTON_8:
            return MouseButton::Button8;
        default:
            return std::nullopt;
    }
}

std::unique_ptr<Window> createWindow() {
    if (!glfwInitialized) {
        glfwInit();
    }

    auto window = std::make_unique<Window>(glfwCreateWindow(640, 480, "", nullptr, nullptr));
    glfwMakeContextCurrent(window->window);

    if (!glfwInitialized) {
        auto res = glewInit();
        if (res != GLEW_OK) {
            throw std::runtime_error("glewInit failed");
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glfwInitialized = true;
    }

    return window;
}

void Window::update() {
    glfwSwapBuffers(window);
    glfwPollEvents();
    if (glfwWindowShouldClose(window)) {
        glfwSetWindowUserPointer(window, nullptr);
        glfwDestroyWindow(window);
        isOpen = false;
    }
}

bool Window::exists() const {
    return isOpen;
}

void Window::setTitle(const char* title) {
    glfwSetWindowTitle(window, title);
}

cr::math::cvector<int, 2> Window::size() const {
    cr::math::cvector<int, 2> size;
    glfwGetWindowSize(window, &size[0], &size[1]);
    return size;
}

std::vector<event> Window::get_events() {
    std::vector<event> events;
    std::swap(events, this->events);
    return events;
}

cr::math::cvector<int, 2> Window::get_mouse_position() const {
    cr::math::cvector<int, 2> pos;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    pos[0] = static_cast<int>(x);
    pos[1] = static_cast<int>(y);
    return pos;
}

Window::Window(GLFWwindow* window) {
    this->window = window;
    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
        auto cr_button = glfw_to_cr_mouse_button(button);
        if (!cr_button) {
            return;
        }
        if (action == GLFW_PRESS) {
            w->events.emplace_back(MousePressEvent{
                    w->get_mouse_position(),
                    *cr_button});
        } else if (action == GLFW_RELEASE) {
            w->events.emplace_back(MouseReleaseEvent{
                    w->get_mouse_position(),
                    *cr_button});
        }
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        auto* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
        // check for pressed buttons -> generate mouse drag event
        for (int i = GLFW_MOUSE_BUTTON_LEFT; i <= GLFW_MOUSE_BUTTON_LAST; i++) {
            if (glfwGetMouseButton(window, i) == GLFW_PRESS) {
                auto cr_button = glfw_to_cr_mouse_button(i);
                if (!cr_button) {
                    continue;
                }
                w->events.emplace_back(MouseDragEvent{
                        w->get_mouse_position(),
                        *cr_button});
            }
        }
        w->events.emplace_back(MouseMoveEvent{
                w->get_mouse_position()});
    });
}

Window::Window(Window&& other) noexcept {
    window = other.window;
    other.window = nullptr;
    isOpen = other.isOpen;
    events = std::move(other.events);
}

Window& Window::operator=(Window&& other) noexcept {
    window = other.window;
    other.window = nullptr;
    isOpen = other.isOpen;
    events = std::move(other.events);
    return *this;
}

Window::~Window() {
    if (window && isOpen) {
        glfwSetWindowUserPointer(window, nullptr);
        glfwDestroyWindow(window);
    }
}

}// namespace cr::ui