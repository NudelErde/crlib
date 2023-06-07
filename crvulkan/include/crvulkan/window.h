//
// Created by nudelerde on 05.06.23.
//

#pragma once

#include "crmath/matrix.h"
#include <cstdint>
#include <memory>

struct GLFWwindow;

namespace cr::vulkan {

struct Window;

std::shared_ptr<Window> createWindow(uint32_t width, uint32_t height);

struct Window : public std::enable_shared_from_this<Window> {
    Window() = default;
    Window(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;
    ~Window();

    [[nodiscard]] uint32_t getWidth() const;
    [[nodiscard]] uint32_t getHeight() const;
    void setTitle(const char* title);

    void pollEvents();
    [[nodiscard]] bool shouldClose() const;
    math::cvector<uint32_t, 2> getFramebufferSize() const;

    GLFWwindow* window = nullptr;

    uint32_t width{};
    uint32_t height{};
};

void closeGlfw();
}// namespace cr::vulkan