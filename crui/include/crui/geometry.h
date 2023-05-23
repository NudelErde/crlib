//
// Created by nudelerde on 19.05.23.
//

#pragma once

#include <variant>
#include <vector>
#include <memory>
#include "crmath/geometry.h"
#include "opengl.h"

namespace cr::ui {
    struct Window;

    struct Point : cr::math::cvector<float, 2> {
        float &x() { return (*this)[0]; }

        float &y() { return (*this)[1]; }

        float x() const { return (*this)[0]; }

        float y() const { return (*this)[1]; }

        Point() = default;

        Point(float x, float y) : cr::math::cvector<float, 2>(x, y) {}
    };

    struct Color : cr::math::cvector<float, 4> {
        float &r() { return (*this)[0]; }

        float &g() { return (*this)[1]; }

        float &b() { return (*this)[2]; }

        float &a() { return (*this)[3]; }

        float r() const { return (*this)[0]; }

        float g() const { return (*this)[1]; }

        float b() const { return (*this)[2]; }

        float a() const { return (*this)[3]; }


        Color() = default;

        Color(float r, float g, float b, float a) : cr::math::cvector<float, 4>(r, g, b, a) {}
    };

    struct Circle {
        Point pos;
        float radius{};
        Color color;
        float innerRadius{};
        float startAngle = 0;
        float endAngle = 2 * std::numbers::pi;
    };
    struct Rectangle {
        Point pos;
        float width{};
        float height{};
        Color color;
    };
    struct Line {
        Point start;
        Point end;
        Color color;
        float strokeWidth{};
    };
    struct Texture {
        Point pos;
        float width{};
        float height{};
        Color color;
        std::shared_ptr<texture> tex;
    };
    struct RawGL {
        std::shared_ptr<shader> shad;
        std::shared_ptr<vertex_array> vao;
        uniform_buffer uniforms;
        std::string windowMatrixName;
        DrawMode mode;
        unsigned int count;
        unsigned int offset;
    };

    using geometry = std::variant<Circle, Rectangle, Line, RawGL>;

    void draw(const std::vector<geometry> &geometries, std::unique_ptr<Window> &window,
              const cr::math::square_matrix<float, 3> &extra_matrix = cr::math::identity<float, 3>());

    void clear(const Color &color, std::unique_ptr<Window> &window);
}
