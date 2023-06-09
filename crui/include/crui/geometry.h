//
// Created by nudelerde on 19.05.23.
//

#pragma once

#include "crmath/geometry.h"
#include "font.h"
#include "opengl.h"
#include <memory>
#include <span>
#include <variant>
#include <vector>

namespace cr::ui {
struct Window;

struct Point : cr::math::cvector<float, 2> {
    [[nodiscard]] float& x() { return (*this)[0]; }

    [[nodiscard]] float& y() { return (*this)[1]; }

    [[nodiscard]] float x() const { return (*this)[0]; }

    [[nodiscard]] float y() const { return (*this)[1]; }

    Point() = default;

    Point(float x, float y) : cr::math::cvector<float, 2>(x, y) {}

    Point(const cr::math::cvector<float, 2>& vec) : cr::math::cvector<float, 2>(vec) {}
};

struct Color : cr::math::cvector<float, 4> {
    [[nodiscard]] float& r() { return (*this)[0]; }

    [[nodiscard]] float& g() { return (*this)[1]; }

    [[nodiscard]] float& b() { return (*this)[2]; }

    [[nodiscard]] float& a() { return (*this)[3]; }

    [[nodiscard]] float r() const { return (*this)[0]; }

    [[nodiscard]] float g() const { return (*this)[1]; }

    [[nodiscard]] float b() const { return (*this)[2]; }

    [[nodiscard]] float a() const { return (*this)[3]; }


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
struct Text {
    Point pos;
    std::shared_ptr<font> font_ptr;
    std::string text;
    Color color;
    float scale;
};

using geometry = std::variant<Circle, Rectangle, Line, RawGL, Text>;

void draw(std::span<const geometry> geometries, std::unique_ptr<Window>& window,
          const cr::math::square_matrix<float, 3>& extra_matrix = cr::math::identity<float, 3>());

inline void draw(const std::vector<geometry>& geometries, std::unique_ptr<Window>& window,
                 const cr::math::square_matrix<float, 3>& extra_matrix = cr::math::identity<float, 3>()) {
    draw(std::span(geometries), window, extra_matrix);
}

void clear(const Color& color, std::unique_ptr<Window>& window);
}// namespace cr::ui
