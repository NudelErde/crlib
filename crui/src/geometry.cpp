//
// Created by nudelerde on 19.05.23.
//

#include "geometry.h"
#include "window.h"
#include "opengl.h"
#include "crutil/overload.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>
#include <cmath>

using namespace cr::ui;

// language=GLSL
static std::string vsSourceCircle = R"(#version 330 core

layout (location = 0) in vec2 position;

uniform mat3 projection;
out vec2 relPos;

void main() {
    vec3 pos = projection * vec3(position, 1.0);
    gl_Position = vec4(pos.xy, 0.0, 1.0);
    relPos = position;
}
)";
// language=GLSL
static std::string fsSourceCircle = R"(#version 330 core

uniform vec4 color;
uniform float innerRadius;
uniform float maxDotProd;
uniform vec2 dotProdVec;
in vec2 relPos;
out vec4 FragColor;

void main() {
    float dist = length(relPos);
    if (dist > 1.0) {
        discard;
    }
    if (dist < innerRadius) {
        discard;
    }
    if (dot(relPos / dist, dotProdVec) < maxDotProd) {
        discard;
    }
    FragColor = color;
})";

// language=GLSL
static std::string vsSourceRectangle = R"(#version 330 core

layout (location = 0) in vec2 position;

uniform mat3 projection;

void main() {
    vec3 pos = projection * vec3(position, 1.0);
    gl_Position = vec4(pos.xy, 0, 1);
}
)";

// language=GLSL
static std::string fsSourceRectangle = R"(#version 330 core

uniform vec4 color;
out vec4 FragColor;

void main() {
    FragColor = color;
}

)";

struct CircleDrawer {
    shader shad;
    vertex_array vao;

    CircleDrawer() {
        shad = create_shader(vsSourceCircle, fsSourceCircle);
        vertex_buffer vbo;
        index_buffer ibo;
        std::array<float, 8> vertices = {
                -1, -1,
                1, -1,
                1, 1,
                -1, 1
        };
        std::array<unsigned int, 6> indices = {
                0, 1, 2,
                2, 3, 0
        };
        vbo.set_data(vertices);
        ibo.set_data(indices);
        vao.set_vertex_buffer(std::move(vbo));
        vao.set_index_buffer(std::move(ibo));
        vao.add_attribute(0, vertex_array::attribute_type::float_type, 2, 0);
    }

    static inline float wrapAngle(float angle) {
        float twoPi = 2 * std::numbers::pi;
        return angle - twoPi * std::floor(angle / twoPi);
    }

    void operator()(const cr::ui::Circle &circle, const cr::math::square_matrix<float, 3> &window_matrix) const {
        cr::math::square_matrix<float, 3> projection =
                window_matrix
                * cr::math::translate_matrix<float>(circle.pos.x(), circle.pos.y())
                * cr::math::scale_matrix<float>(cr::math::with_translation, circle.radius, circle.radius);
        auto startAngle = wrapAngle(circle.startAngle);
        auto angle = wrapAngle(circle.endAngle - circle.startAngle);
        if (startAngle == angle) {
            angle += 2 * std::numbers::pi;
        }
        // find direction vector of startAngle + angle / 2
        auto midAngle = startAngle + angle / 2;
        auto dotProdVec = cr::math::cvector<float, 2>(cos(midAngle), sin(midAngle));
        // find max dot product of dotProdVec with any point on the circle
        auto maxDotProd = std::cos(angle / 2);
        uniform_buffer uniforms = {
                {"projection",  projection},
                {"color",       circle.color},
                {"innerRadius", circle.innerRadius / circle.radius},
                {"maxDotProd",  maxDotProd},
                {"dotProdVec",  dotProdVec},
        };
        draw(vao, shad, uniforms, 6, 0);
    }
};

struct RectangleDrawer {
    shader shad;
    vertex_array vao;

    RectangleDrawer() {
        shad = create_shader(vsSourceRectangle, fsSourceRectangle);
        vertex_buffer vbo;
        index_buffer ibo;
        std::array<float, 8> vertices = {
                0, 0,
                1, 0,
                1, 1,
                0, 1
        };
        std::array<unsigned int, 6> indices = {
                0, 1, 2,
                2, 3, 0
        };
        vbo.set_data(vertices);
        ibo.set_data(indices);
        vao.set_vertex_buffer(std::move(vbo));
        vao.set_index_buffer(std::move(ibo));
        vao.add_attribute(0, vertex_array::attribute_type::float_type, 2, 0);
    }

    void operator()(const cr::ui::Rectangle &rect, const cr::math::square_matrix<float, 3> &window_matrix) const {
        cr::math::square_matrix<float, 3> projection =
                window_matrix *
                cr::math::translate_matrix<float>(rect.pos.x(), rect.pos.y()) *
                cr::math::scale_matrix<float>(cr::math::with_translation, rect.width, rect.height);
        uniform_buffer uniforms = {
                {"projection", projection},
                {"color",      rect.color}
        };
        draw(vao, shad, uniforms, 6, 0);
    }
};

struct LineDrawer {
    shader shad;
    vertex_array vao;

    LineDrawer() {
        shad = create_shader(vsSourceRectangle, fsSourceRectangle);
        vertex_buffer vbo;
        index_buffer ibo;
        std::array<float, 4> vertices = {
                0, 0,
                1, 1,
        };
        std::array<unsigned int, 2> indices = {
                0, 1
        };
        vbo.set_data(vertices);
        ibo.set_data(indices);
        vao.set_vertex_buffer(std::move(vbo));
        vao.set_index_buffer(std::move(ibo));
        vao.add_attribute(0, vertex_array::attribute_type::float_type, 2, 0);
    }

    void operator()(const cr::ui::Line &line, const cr::math::square_matrix<float, 3> &window_matrix) const {
        auto delta = line.end - line.start;
        cr::math::square_matrix<float, 3> projection =
                window_matrix *
                cr::math::translate_matrix<float>(line.start.x(), line.start.y()) *
                cr::math::scale_matrix<float>(cr::math::with_translation, delta[0], delta[1]);
        uniform_buffer uniforms = {
                {"projection", projection},
                {"color",      line.color}
        };
        lineWidth(line.strokeWidth);
        draw(vao, shad, uniforms, 2, 0, DrawMode::lines);
    }
};

namespace cr::ui {
    static auto &get_visitor() {
        static auto visitor = cr::util::overloaded{
                CircleDrawer{},
                RectangleDrawer{},
                LineDrawer{},
                [](const RawGL &raw, const cr::math::square_matrix<float, 3> &window_matrix) {
                    uniform_buffer tmp = raw.uniforms;
                    tmp.push_back({raw.windowMatrixName, window_matrix});
                    draw(*raw.vao, *raw.shad, tmp, (int) raw.count, (int) raw.offset, raw.mode);
                }
        };
        return visitor;
    }

    void draw(const std::vector<geometry> &geometries, std::unique_ptr<Window> &window, const cr::math::square_matrix<float, 3> &extra_matrix) {
        if (!window->exists())
            return;
        glfwMakeContextCurrent(window->window);
        auto size = window->size();
        glViewport(0, 0, size[0], size[1]);
        auto opengl_window_to_pixel_matrix = opengl_window_to_pixel(size[0], size[1]) * extra_matrix;
        for (const auto &geometry: geometries) {
            std::visit(get_visitor(), geometry,
                       std::variant<cr::math::matrix<float, 3, 3>>(opengl_window_to_pixel_matrix));
        }
    }

    void clear(const Color &color, std::unique_ptr<Window> &window) {
        if (!window->exists())
            return;
        glfwMakeContextCurrent(window->window);
        clear(color);
    }
}