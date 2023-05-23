//
// Created by nudelerde on 19.05.23.
//

#include <iostream>
#include <vector>
#include "crui/window.h"
#include "crui/geometry.h"

using namespace cr::ui;

auto rk4(double x0, double t, auto func, double h, std::vector<geometry> &lines, bool drawHelperLines, const Color &c) {
    auto k1 = h * func(x0, t);
    auto k2 = h * func(x0 + 0.5 * k1, t + 0.5 * h);
    auto k3 = h * func(x0 + 0.5 * k2, t + 0.5 * h);
    auto k4 = h * func(x0 + k3, t + h);

    auto x1 = x0 + (k1 + 2 * k2 + 2 * k3 + k4) / 6.0;

    if (drawHelperLines) {
        lines.emplace_back(Line{{float(t), float(x0)}, {float(t + h), float(k1)}, {1, 0, 0, 0.2}, 4});
        lines.emplace_back(Line{{float(t), float(x0)}, {float(t + h), float(k2)}, {0, 1, 0, 0.2}, 4});
        lines.emplace_back(Line{{float(t), float(x0)}, {float(t + h), float(k3)}, {0, 0, 1, 0.2}, 4});
        lines.emplace_back(Line{{float(t), float(x0)}, {float(t + h), float(k4)}, {1, 1, 0, 0.2}, 4});
    }
    lines.emplace_back(Line{{float(t), float(x0)}, {float(t + h), float(x1)}, c, 4});

    return x1;
}

int main() {
    auto window = createWindow();
    window->setTitle("Test");
/*
    float time = 0;

    while (window->exists()) {
        time += 0.01;
        cr::ui::Color color{0, 0, 0, 1};
        cr::ui::clear(color, window);
        cr::ui::draw({
                             cr::ui::Line{{0, 0}, {100, 100}, {1, 0, 0, 1}, 1},
                             cr::ui::Line{{100, 100}, {200, 0}, {0, 1, 0, 1}, 1},
                             cr::ui::Line{{200, 0}, {0, 0}, {0, 0, 1, 1}, 1},
                     }, window);
        cr::ui::draw({
                             cr::ui::Circle{{300, 300}, 50, {1, 1, 0, 1}},
                             cr::ui::Circle{{275, 285}, 10, {0, 0, 0, 1}},
                             cr::ui::Circle{{325, 285}, 10, {0, 0, 0, 1}},
                             cr::ui::Circle{{300, 300}, 40, {0, 0, 0, 1}, 30,
                                            std::numbers::pi * 1 / 4, std::numbers::pi * 3 / 4},
                     }, window, cr::math::rotation_matrix_at<float>(time, 300, 300));
        window->update();
    }*/

    double t = 0;

    while (window->exists()) {
        auto size = window->size();
        clear({.8, .8, .8, 1}, window);

        t += 0.02;

        std::vector<geometry> lines;
        lines.reserve(10);

        auto func = [](double x, double t) {
            return 5.0 / 2.0 * std::cos(2.0 * t + 1.0) - 1.0 / 8.0 * x + 1.0 / 2.0;
        };

        double x = 5 * std::sin(t);
        size_t width = 16;

        for (size_t i = 0; i < width; ++i) {
            double h = 2.0;
            x = rk4(x, double(i) * h, func, h, lines, false, {0, 0, 0, 1});
        }

        x = 5 * std::sin(t);

        for (size_t i = 0; i < width * 100; ++i) {
            double h = 2.0 / 100.0;
            x = rk4(x, double(i) * h, func, h, lines, false, {0, 0.7, 0, 1});
        }

        draw(lines, window,
             cr::math::translate_matrix<float>(0, size[1] / 2) *
             cr::math::scale_matrix<float>(cr::math::with_translation, 30, -30));
        window->update();
    }

    return 0;
}