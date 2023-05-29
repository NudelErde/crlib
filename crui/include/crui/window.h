//
// Created by nudelerde on 19.05.23.
//

#pragma once

#include "crmath/matrix.h"
#include <memory>
#include <variant>
#include <vector>

struct GLFWwindow;

namespace cr::ui {
enum class MouseButton {
    Left,
    Right,
    Middle,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8
};
struct MouseMoveEvent {
    cr::math::cvector<int, 2> pos;
};
struct MouseDragEvent {
    cr::math::cvector<int, 2> pos;
    MouseButton button;
};
struct MousePressEvent {
    cr::math::cvector<int, 2> pos;
    MouseButton button;
};
struct MouseReleaseEvent {
    cr::math::cvector<int, 2> pos;
    MouseButton button;
};
using event = std::variant<MouseMoveEvent, MousePressEvent, MouseReleaseEvent, MouseDragEvent>;

struct Window {
    GLFWwindow* window = nullptr;

    explicit Window(GLFWwindow* window);
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;
    ~Window();

    void update();
    std::vector<event> get_events();

    [[nodiscard]] cr::math::cvector<int, 2> size() const;

    void setTitle(const char* title);

    [[nodiscard]] bool exists() const;

    [[nodiscard]] cr::math::cvector<int, 2> get_mouse_position() const;

    [[nodiscard]] bool is_mouse_button_pressed(MouseButton button) const;

private:
    bool isOpen = true;
    std::vector<event> events;

    friend std::unique_ptr<Window> createWindow();
};

std::unique_ptr<Window> createWindow();
}// namespace cr::ui
