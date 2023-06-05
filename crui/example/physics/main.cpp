//
// Created by nudelerde on 19.05.23.
//

#include "crui/font.h"
#include "crui/geometry.h"
#include "crui/gui.h"
#include "crui/window.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

using namespace cr;

struct hsv {
    double h;// angle in degrees
    double s;// a fraction between 0 and 1
    double v;// a fraction between 0 and 1
};

ui::Color hsv2rgb(hsv in) {
    double hh, p, q, t, ff;
    long i;
    ui::Color out;

    if (in.s <= 0.0) {// < is bogus, just shuts up warnings
        out[0] = in.v;
        out[1] = in.v;
        out[2] = in.v;
        return out;
    }
    hh = in.h;
    if (hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long) hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch (i) {
        case 0:
            out[0] = in.v;
            out[1] = t;
            out[2] = p;
            break;
        case 1:
            out[0] = q;
            out[1] = in.v;
            out[2] = p;
            break;
        case 2:
            out[0] = p;
            out[1] = in.v;
            out[2] = t;
            break;

        case 3:
            out[0] = p;
            out[1] = q;
            out[2] = in.v;
            break;
        case 4:
            out[0] = t;
            out[1] = p;
            out[2] = in.v;
            break;
        case 5:
        default:
            out[0] = in.v;
            out[1] = p;
            out[2] = q;
            break;
    }
    return out;
}

void draw_physics(std::unique_ptr<ui::Window>& window, math::cvector<float, 2>& state) {
    float wheel_radius = 50;
    ui::Point wheel_pos{149, 100};
    ui::Color string_color{1, 1, 0, 1};
    ui::Color wheel_color{0, 0, 0, 1};
    ui::Color object_color{1, 0, 0, 1};
    ui::Color spring_color{0, 0, 0, 1};
    float spring_vis_strength = 20;

    float obj_pos = state[0] * 20;

    float angle = obj_pos / wheel_radius;

    ui::Point spring_end{50 + obj_pos, wheel_pos.y() - wheel_radius};
    ui::Point spring_start{10, spring_end.y()};
    ui::Point object_pos{wheel_pos + math::cvector<float, 2>{1 + wheel_radius, 100 + obj_pos}};

    ui::draw({
                     //string
                     ui::Line{wheel_pos + ui::Point{0, -wheel_radius}, spring_end,
                              string_color, 3},
                     ui::Line{{200, 100}, object_pos, string_color, 3},
                     ui::Circle{wheel_pos, wheel_radius + 1, string_color, wheel_radius - 1, -std::numbers::pi / 2,
                                0},
                     //spring
                     ui::Rectangle{{-10, wheel_pos.y() - wheel_radius - 10}, 20, 20, {0, 0, 0, 1}},
                     ui::Line{spring_start,
                              spring_start * 0.8 + spring_end * 0.2 + ui::Point{0, spring_vis_strength},
                              spring_color,
                              3},
                     ui::Line{spring_start * 0.8 + spring_end * 0.2 + ui::Point{0, spring_vis_strength},
                              spring_start * 0.6 + spring_end * 0.4 + ui::Point{0, -spring_vis_strength},
                              spring_color, 3},
                     ui::Line{spring_start * 0.6 + spring_end * 0.4 + ui::Point{0, -spring_vis_strength},
                              spring_start * 0.4 + spring_end * 0.6 + ui::Point{0, spring_vis_strength},
                              spring_color, 3},
                     ui::Line{spring_start * 0.4 + spring_end * 0.6 + ui::Point{0, spring_vis_strength},
                              spring_start * 0.2 + spring_end * 0.8 + ui::Point{0, -spring_vis_strength},
                              spring_color, 3},
                     ui::Line{spring_start * 0.2 + spring_end * 0.8 + ui::Point{0, -spring_vis_strength},
                              spring_end,
                              spring_color, 3},
                     //wheel
                     ui::Line{wheel_pos,
                              math::rotation_matrix(angle) * math::cvector<float, 2>(0, wheel_radius) + wheel_pos,
                              wheel_color, 2},
                     ui::Line{wheel_pos,
                              math::rotation_matrix(angle) * math::cvector<float, 2>(wheel_radius, 0) + wheel_pos,
                              wheel_color, 2},
                     ui::Line{wheel_pos,
                              math::rotation_matrix(angle) * math::cvector<float, 2>(0, -wheel_radius) + wheel_pos,
                              wheel_color, 2},
                     ui::Line{wheel_pos,
                              math::rotation_matrix(angle) * math::cvector<float, 2>(-wheel_radius, 0) + wheel_pos,
                              wheel_color, 2},
                     ui::Circle{wheel_pos, wheel_radius, wheel_color, wheel_radius - 4},
                     //object
                     ui::Circle{object_pos, 10, object_color},
             },
             window);
}

void do_slider(double& value, double min, double max, const std::string& text, const ui::Point& p,
               std::shared_ptr<ui::font>& font, std::unique_ptr<ui::Window>& window, const std::vector<ui::event>& events) {
    ui::Slider slider(p, {150, 20}, float(value - min) / float(max - min),
                      {0.6, 0.6, 0.6, 1}, {0, 0, 0.8, 1}, 2);
    apply_all(events, slider);
    update(slider, window, true);
    value = slider.get_value() * (max - min) + min;
    ui::draw({ui::Text{p + ui::Point{160, 20},
                       font,
                       text + " " + std::to_string(value),
                       {1, 1, 1, 1},
                       .125}},
             window);
}

int main(int argc, char** argv) {

    double m = 0.5;
    double r = 1;
    double I = 0.5;
    double kf = 4;
    double kd = 0;
    double g = 9.81;

    char const* font_path = "/usr/share/fonts/carlito/Carlito-Regular.ttf";

    if (argc < 2) {
        std::cout << "Assume default font located at /usr/share/fonts/carlito/Carlito-Regular.ttf\n";
        std::cout << "Usage: " << argv[0] << " <font path>\n";
    } else {
        std::cout << "Use font located at " << argv[1] << "\n";
        font_path = argv[1];
    }

    auto window = ui::createWindow();
    window->setTitle("Test");

    auto font = ui::loadFont(font_path, 40);

    auto step_func = [&](const math::cvector<float, 2>& state) {
        double c1 = (m * r) / ((I / r) + m * r);
        double c2 = (r * kf) / ((I / r) + m * r);
        double c3 = (1) / (I + m * r * r);
        math::square_matrix<float, 2> A{0, 1, -c2, -c3 * kd};
        math::cvector<float, 2> b{0, c1 * g};
        return A * state + b;
    };

    math::cvector<float, 2> state{0, 0};
    auto time = std::chrono::high_resolution_clock::now();

    while (window->exists()) {
        auto dt = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - time).count();
        time = std::chrono::high_resolution_clock::now();

        ui::clear(ui::Color{0.2, 0.2, 0.2, 1}, window);
        ui::draw({
                         ui::Rectangle{{0, 0}, 225, float(window->size()[1]) - 150.0f, {0.4, 0.4, 0.4, 1}},
                 },
                 window);
        draw_physics(window, state);
        auto events = window->get_events();

        do_slider(g, 0, 18, "g:", {250, 25}, font, window, events);
        do_slider(kd, 0, 5, "kd:", {250, 50}, font, window, events);
        do_slider(m, 0, 2, "m:", {250, 75}, font, window, events);
        do_slider(r, 0.01, 2, "r:", {250, 100}, font, window, events);
        do_slider(I, 0.01, 2, "I:", {250, 125}, font, window, events);
        do_slider(kf, 0, 10, "kf:", {250, 150}, font, window, events);

        double c1 = (m * r) / ((I / r) + m * r);
        double c2 = (r * kf) / ((I / r) + m * r);
        double c3 = (1) / (I + m * r * r);
        ui::draw({
                         ui::Text{{20, float(window->size()[1]) - 100.0f}, font, "c1: " + std::to_string(c1),//
                                  {1, 1, 1, 1},
                                  .125},
                         ui::Text{{20, float(window->size()[1]) - 75.0f}, font, "c2: " + std::to_string(c2),//
                                  {1, 1, 1, 1},
                                  .125},
                         ui::Text{{20, float(window->size()[1]) - 50.0f}, font, "c3: " + std::to_string(c3),//
                                  {1, 1, 1, 1},
                                  .125},
                 },
                 window);

        auto m_pos = window->get_mouse_position();
        if (m_pos[0] < 225 && float(m_pos[1]) < float(window->size()[1]) - 150.0f && window->is_mouse_button_pressed(ui::MouseButton::Left)) {
            state[0] = (float(m_pos[1]) - 200.0f) / 20.0f;
        } else if (m_pos[0] < 550 && m_pos[0] > 250 && m_pos[1] < 475 && m_pos[1] > 175 && window->is_mouse_button_pressed(ui::MouseButton::Left)) {
            math::cvector<float, 2> tmp = m_pos;
            state = (tmp - ui::Point{250.0f + 150.0f, 175.0f + 150.0f}) / 20.0f;
        } else {
            state = math::rk4(state, dt, step_func);
        }

        ui::draw({
                         ui::Rectangle{{250, 175}, 300, 300, {0.4, 0.4, 0.4, 1}},
                         ui::Line{{250 + 150, 175}, {250 + 150, 175 + 300}, {0, 0, 0, 1}, 2},
                         ui::Line{{250, 175 + 150}, {250 + 300, 175 + 150}, {0, 0, 0, 1}, 2},
                         ui::Text{{250 + 150 + 4, 175 + 15}, font, "x'", {1, 1, 1, 1}, .125},
                         ui::Text{{250 + 300 - 10, 175 + 150 - 5}, font, "x", {1, 1, 1, 1}, .125},
                         ui::Circle{state * 20 + ui::Point{250 + 150, 175 + 150}, 5, {1, 1, 0, 1}},
                 },
                 window);
        double width = 300.0 / 20.0;
        double height = 300.0 / 20.0;
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
                // use width
                double s0 = (i - 10 + 0.5) / 20.0 * width;
                double s1 = (j - 10 + 0.5) / 20.0 * height;
                math::cvector<float, 2> s{s0, s1};
                math::cvector<float, 2> s_dot = step_func(s);
                auto diff = s_dot - s;
                auto len = length(diff);
                if (len == 0) continue;
                auto direction = diff / len * 0.5;
                auto color = hsv2rgb({len * 4, 0.8, 0.8});
                color[3] = 1;
                ui::draw({
                                 ui::Circle{s * 20 + ui::Point{250 + 150, 175 + 150}, 2, color},
                                 ui::Line{s * 20 + ui::Point{250 + 150, 175 + 150},
                                          (s + direction) * 20 + ui::Point{250 + 150, 175 + 150}, color, 1},
                         },
                         window);
            }
        }

        window->update();
    }

    return 0;
}