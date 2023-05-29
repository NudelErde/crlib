//
// Created by nudelerde on 29.05.23.
//

#pragma once

#include "geometry.h"
#include "window.h"
#include <span>
#include <utility>

namespace cr::ui {

struct Slider {

    Slider() = default;
    Slider(Point pos, math::cvector<float, 2> size, float value, const Color& background_color,
           const Color& foreground_color, float border_width) : pos(std::move(pos)), size(std::move(size)), value(value),
                                                                background_color(background_color),
                                                                foreground_color(foreground_color),
                                                                border_width(border_width) {}

    [[nodiscard]] const Point& get_pos() const;
    void set_pos(const Point& pos);
    [[nodiscard]] const math::cvector<float, 2>& get_size() const;
    void set_size(const math::cvector<float, 2>& size);
    [[nodiscard]] float get_value() const;
    void set_value(float value);
    [[nodiscard]] const Color& get_background_color() const;
    void set_background_color(const Color& background_color);
    [[nodiscard]] const Color& get_foreground_color() const;
    void set_foreground_color(const Color& foreground_color);
    [[nodiscard]] float get_border_width() const;
    void set_border_width(float border_width);

    void apply(const event& event);
    [[nodiscard]] bool requires_redraw() const;
    [[nodiscard]] std::span<const geometry> get_geometry();

private:
    Point pos;
    math::cvector<float, 2> size;
    float value{};
    bool stateChanged = true;
    Color background_color;
    Color foreground_color;
    float border_width = 1.0f;

    void update_geometry();

    std::vector<geometry> geometries;
};

template<typename T>
void apply(const event& e, T& g) {
    g.apply(e);
}

template<typename Events, typename T>
void apply_all(Events& cont, T& g) {
    for (const auto& e : cont) {
        apply(e, g);
    }
}

template<typename T>
void update(T& g, std::unique_ptr<Window>& window, bool force = false, const cr::math::square_matrix<float, 3>& extra_matrix = cr::math::identity<float, 3>()) {
    if (force || g.requires_redraw()) {
        draw(g.get_geometry(), window, extra_matrix);
    }
}

template<typename Container>
void update_all(Container& cont, std::unique_ptr<Window>& window, bool force = false, const cr::math::square_matrix<float, 3>& extra_matrix = cr::math::identity<float, 3>()) {
    for (auto& element : cont) {
        update(element, window, force, extra_matrix);
    }
}

}// namespace cr::ui