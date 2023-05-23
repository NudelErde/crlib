//
// Created by nudelerde on 21.05.23.
//

#pragma once

#include <variant>
#include <vector>
#include "crmath/geometry.h"

namespace cr::ui {

    using uniform_value = std::variant<float, int, bool,
            cr::math::square_matrix<float, 4>,
            cr::math::square_matrix<float, 3>,
            cr::math::square_matrix<float, 2>,
            cr::math::cvector<float, 2>,
            cr::math::cvector<float, 3>,
            cr::math::cvector<float, 4>>;

    struct uniform {
        std::string name;
        uniform_value value;
    };

    using uniform_buffer = std::vector<uniform>;

    struct shader {
        shader();

        shader(const shader &) = delete;

        shader(shader &&) noexcept;

        shader &operator=(const shader &) = delete;

        shader &operator=(shader &&) noexcept;

        ~shader();

        void set_uniform(const std::string &name, const uniform_value &value) const;

        void use() const;

    private:
        unsigned int id{};

        friend shader create_shader(const std::string &vertex_shader, const std::string &fragment_shader);
    };

    shader create_shader(const std::string &vertex_shader, const std::string &fragment_shader);

    struct vertex_buffer {
        vertex_buffer();

        vertex_buffer(const vertex_buffer &) = delete;

        vertex_buffer(vertex_buffer &&) noexcept;

        vertex_buffer &operator=(const vertex_buffer &) = delete;

        vertex_buffer &operator=(vertex_buffer &&) noexcept;

        ~vertex_buffer();

        template<typename T>
        void set_data(const T& t) const {
            set_data(t.data(), t.size() * sizeof(typename T::value_type));
        }

        void set_data(const void *data, unsigned int size) const;

    private:
        unsigned int id{};

        friend struct vertex_array;
    };

    struct index_buffer {
        index_buffer();

        index_buffer(const index_buffer &) = delete;

        index_buffer(index_buffer &&) noexcept;

        index_buffer &operator=(const index_buffer &) = delete;

        index_buffer &operator=(index_buffer &&) noexcept;

        ~index_buffer();

        template<typename T>
        void set_data(const T& t) const {
            set_data(t.data(), t.size() * sizeof(typename T::value_type));
        }

        void set_data(const void *data, unsigned int size) const;

    private:
        unsigned int id{};

        friend struct vertex_array;
    };

    struct vertex_array {
        vertex_array();

        vertex_array(const vertex_array &) = delete;

        vertex_array(vertex_array &&) noexcept;

        vertex_array &operator=(const vertex_array &) = delete;

        vertex_array &operator=(vertex_array &&) noexcept;

        ~vertex_array();

        enum class attribute_type {
            float_type,
            int_type,
            byte_type
        };

        void set_vertex_buffer(vertex_buffer &&vertex_buffer);

        void set_index_buffer(index_buffer &&index_buffer);

        /**
         * @brief sets an attribute for the vertex array
         * @param index location of the attribute in the shader
         * @param type type of the attribute
         * @param count number of elements of the attribute (1, 2, 3 or 4)
         * @param offset offset of the attribute in the vertex
         * @return offset of the next attribute
         */
        unsigned int add_attribute(unsigned int index, attribute_type type, int count, unsigned int offset);

        void use() const;

    private:
        unsigned int id{};
        vertex_buffer vertexBuffer{};
        index_buffer indexBuffer{};
    };

    struct texture {
        
    };

    enum class DrawMode {
        points,
        line_strip,
        line_loop,
        lines,
        triangle_strip,
        triangle_fan,
        triangles
    };

    void lineWidth(float width);
    void draw(const vertex_array &vertex_array, const shader &shader, const uniform_buffer &uniform_buffer, int count,
              int offset = 0, DrawMode mode = DrawMode::triangles);

    void clear(const cr::math::cvector<float, 4> &color);

    cr::math::square_matrix<float, 3> opengl_window_to_pixel(int width, int height);

}
