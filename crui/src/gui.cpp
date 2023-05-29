//
// Created by nudelerde on 29.05.23.
//

#include "gui.h"

namespace cr::ui {

[[nodiscard]] const Point& Slider::get_pos() const {
    return pos;
}
void Slider::set_pos(const Point& n_pos) {
    stateChanged = true;
    pos = n_pos;
}
[[nodiscard]] const math::cvector<float, 2>& Slider::get_size() const {
    return size;
}
void Slider::set_size(const math::cvector<float, 2>& n_size) {
    stateChanged = true;
    size = n_size;
}
[[nodiscard]] float Slider::get_value() const {
    return value;
}
void Slider::set_value(float n_value) {
    stateChanged = true;
    value = n_value;
}
[[nodiscard]] const Color& Slider::get_background_color() const {
    return background_color;
}
void Slider::set_background_color(const Color& n_background_color) {
    stateChanged = true;
    background_color = n_background_color;
}
[[nodiscard]] const Color& Slider::get_foreground_color() const {
    return foreground_color;
}
void Slider::set_foreground_color(const Color& n_foreground_color) {
    stateChanged = true;
    foreground_color = n_foreground_color;
}
[[nodiscard]] float Slider::get_border_width() const {
    return border_width;
}
void Slider::set_border_width(float n_border_width) {
    stateChanged = true;
    border_width = n_border_width;
}

void Slider::apply(const event& event) {
    if (auto* mouseDrag = std::get_if<MouseDragEvent>(&event)) {
        if (mouseDrag->button == MouseButton::Left) {
            math::cvector<float, 2> mousePos{mouseDrag->pos};
            if (mousePos[1] < pos[1] || mousePos[1] > pos[1] + size[1])
                return;
            float x = mousePos[0] - pos[0];
            if (x < 0) return;
            if (x > size[0]) return;
            x -= border_width;
            float v = x / (size[0] - 2 * border_width);
            if (v > 1.0f)
                v = 1.0f;
            if (v < 0.0f)
                v = 0.0f;
            value = v;
            stateChanged = true;
        }
    } else if (auto* mousePress = std::get_if<MousePressEvent>(&event)) {
        if (mousePress->button == MouseButton::Left) {
            math::cvector<float, 2> mousePos{mousePress->pos};
            if (mousePos[1] < pos[1] || mousePos[1] > pos[1] + size[1])
                return;
            float x = mousePos[0] - pos[0];
            if (x < 0) return;
            if (x > size[0]) return;
            x -= border_width;
            float v = x / (size[0] - 2 * border_width);
            if (v > 1.0f)
                v = 1.0f;
            if (v < 0.0f)
                v = 0.0f;
            value = v;
            stateChanged = true;
        }
    }
}
void Slider::update_geometry() {
    geometries.clear();
    geometries.reserve(2);
    geometries.emplace_back(Rectangle{pos, size[0], size[1], background_color});
    geometries.emplace_back(Rectangle{pos + math::cvector<float, 2>{border_width, border_width},
                                      (size[0] - 2 * border_width) * value, size[1] - 2 * border_width, foreground_color});
    stateChanged = false;
}

[[nodiscard]] bool Slider::requires_redraw() const {
    return stateChanged;
}
[[nodiscard]] std::span<const geometry> Slider::get_geometry() {
    if (stateChanged) {
        update_geometry();
    }
    return geometries;
}


}// namespace cr::ui
