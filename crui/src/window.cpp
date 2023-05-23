//
// Created by nudelerde on 19.05.23.
//

#include "window.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

static bool glfwInitialized = false;

namespace cr::ui {
    std::unique_ptr<Window> createWindow() {
        if (!glfwInitialized) {
            glfwInit();
        }

        auto window = std::make_unique<Window>();
        window->window = glfwCreateWindow(640, 480, "", nullptr, nullptr);
        glfwMakeContextCurrent(window->window);

        if (!glfwInitialized) {
            auto res = glewInit();
            if (res != GLEW_OK) {
                throw std::runtime_error("glewInit failed");
            }
            glfwInitialized = true;
        }

        return window;
    }

    void Window::update() {
        glfwSwapBuffers(window);
        glfwPollEvents();
        if (glfwWindowShouldClose(window)) {
            glfwDestroyWindow(window);
            isOpen = false;
        }
    }

    bool Window::exists() const {
        return isOpen;
    }

    void Window::setTitle(const char *title) {
        glfwSetWindowTitle(window, title);
    }

    cr::math::cvector<int, 2> Window::size() const {
        cr::math::cvector<int, 2> size;
        glfwGetWindowSize(window, &size[0], &size[1]);
        return size;
    }
}