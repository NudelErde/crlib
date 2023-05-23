//
// Created by nudelerde on 19.05.23.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <cstring>
#include "crui/window.h"
#include "crui/geometry.h"

using namespace cr;

int main(int argc, char **argv) {
    double c1 = 0.5;
    double c2 = 4;
    double c3 = 1;
    double kd = 0;
    double g = 9.81;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-c1") == 0 && i + 1 < argc) {
                c1 = std::stod(argv[i + 1]);
            } else if (strcmp(argv[i], "-c2") == 0 && i + 1 < argc) {
                c2 = std::stod(argv[i + 1]);
            } else if (strcmp(argv[i], "-c3") == 0 && i + 1 < argc) {
                c3 = std::stod(argv[i + 1]);
            } else if (strcmp(argv[i], "-kd") == 0 && i + 1 < argc) {
                kd = std::stod(argv[i + 1]);
            } else if (strcmp(argv[i], "-g") == 0 && i + 1 < argc) {
                g = std::stod(argv[i + 1]);
            } else if (strcmp(argv[i], "-h") == 0) {
                std::cout << "Usage: " << argv[0] << " [-c1 <c1>] [-c2 <c2>] [-c3 <c3>] [-kd <kd>] [-g <g>]"
                          << std::endl;
                return 0;
            }
        }
    }

    auto window = ui::createWindow();
    window->setTitle("Test");

    std::stringstream ss;
    ss << "c1: " << c1 << " c2: " << c2 << " c3: " << c3 << " kd: " << kd;
    window->setTitle(ss.str().c_str());

    math::square_matrix<float, 2> A{0, 1, -c2, -c3 * kd};
    math::cvector<float, 2> b{0, c1 * g};

    auto step_func = [&](const math::cvector<float, 2> &state) {
        return A * state + b;
    };

    math::cvector<float, 2> state{0, 0};
    auto time = std::chrono::high_resolution_clock::now();

    while (window->exists()) {
        auto dt = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - time).count();
        time = std::chrono::high_resolution_clock::now();
        state = math::rk4(state, dt, step_func);

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

        ui::clear(ui::Color{0.2, 0.2, 0.2, 1}, window);
        ui::draw({
                         //string
                         ui::Line{wheel_pos + ui::Point{0, -wheel_radius}, spring_end,
                                  string_color, 3},
                         ui::Line{{200, 100}, {200, 200 + obj_pos}, string_color, 3},
                         ui::Circle{wheel_pos, wheel_radius + 1, string_color, wheel_radius - 1, -std::numbers::pi / 2,
                                    0},
                         //spring
                         ui::Rectangle{{-10, wheel_pos.y() - wheel_radius - 10}, 20, 20},
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
                         ui::Circle{{200, 200 + obj_pos}, 10, object_color},
                 }, window);

        window->update();
    }

    return 0;
}