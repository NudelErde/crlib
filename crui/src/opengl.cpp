//
// Created by nudelerde on 21.05.23.
//

#include "opengl.h"
#include "crutil/overload.h"
#include <GL/glew.h>

namespace cr::ui {

    shader::shader() {
        id = glCreateProgram();
    }

    shader::shader(shader &&other) noexcept {
        id = other.id;
        other.id = 0;
    }

    shader &shader::operator=(shader &&other) noexcept {
        if (id) glDeleteProgram(id);
        id = other.id;
        other.id = 0;
        return *this;
    }

    shader::~shader() {
        if (id) glDeleteProgram(id);
    }

    void shader::set_uniform(const std::string &name, const uniform_value &value) const {
        auto location = glGetUniformLocation(id, name.c_str());
        std::visit(util::overloaded{
                [&](float v) { glUniform1f(location, v); },
                [&](int v) { glUniform1i(location, v); },
                [&](bool v) { glUniform1i(location, v); },
                [&](const cr::math::square_matrix<float, 4> &v) { glUniformMatrix4fv(location, 1, GL_TRUE, v.raw()); },
                [&](const cr::math::square_matrix<float, 3> &v) { glUniformMatrix3fv(location, 1, GL_TRUE, v.raw()); },
                [&](const cr::math::square_matrix<float, 2> &v) { glUniformMatrix2fv(location, 1, GL_TRUE, v.raw()); },
                [&](const cr::math::cvector<float, 2> &v) { glUniform2fv(location, 1, v.raw()); },
                [&](const cr::math::cvector<float, 3> &v) { glUniform3fv(location, 1, v.raw()); },
                [&](const cr::math::cvector<float, 4> &v) { glUniform4fv(location, 1, v.raw()); },
        }, value);
    }

    void shader::use() const {
        glUseProgram(id);
    }

    shader create_shader(const std::string &vertex_shader, const std::string &fragment_shader) {
        shader s;
        auto vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        auto fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
        auto vertex_shader_source = vertex_shader.c_str();
        auto fragment_shader_source = fragment_shader.c_str();
        glShaderSource(vertex_shader_id, 1, &vertex_shader_source, nullptr);
        glShaderSource(fragment_shader_id, 1, &fragment_shader_source, nullptr);
        glCompileShader(vertex_shader_id);
        glCompileShader(fragment_shader_id);
        // check for errors
        int success;
        char info_log[512];
        glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex_shader_id, 512, nullptr, info_log);
            throw std::runtime_error("vertex shader compilation failed: " + std::string(info_log));
        }
        glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragment_shader_id, 512, nullptr, info_log);
            throw std::runtime_error("fragment shader compilation failed: " + std::string(info_log));
        }
        glAttachShader(s.id, vertex_shader_id);
        glAttachShader(s.id, fragment_shader_id);
        glLinkProgram(s.id);
        glGetProgramiv(s.id, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(s.id, 512, nullptr, info_log);
            throw std::runtime_error("shader linking failed: " + std::string(info_log));
        }
        glDeleteShader(vertex_shader_id);
        glDeleteShader(fragment_shader_id);
        return s;
    }

    vertex_buffer::vertex_buffer() {
        glCreateBuffers(1, &id);
    }

    vertex_buffer::vertex_buffer(vertex_buffer &&other) noexcept {
        id = other.id;
        other.id = 0;
    }

    vertex_buffer &vertex_buffer::operator=(vertex_buffer &&other) noexcept {
        if (id) glDeleteBuffers(1, &id);
        id = other.id;
        other.id = 0;
        return *this;
    }

    vertex_buffer::~vertex_buffer() {
        if (id) glDeleteBuffers(1, &id);
    }

    void vertex_buffer::set_data(const void *data, unsigned int size) const {
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }

    index_buffer::index_buffer() {
        glCreateBuffers(1, &id);
    }

    index_buffer::index_buffer(index_buffer &&other) noexcept {
        id = other.id;
        other.id = 0;
    }

    index_buffer &index_buffer::operator=(index_buffer &&other) noexcept {
        if (id) glDeleteBuffers(1, &id);
        id = other.id;
        other.id = 0;
        return *this;
    }

    index_buffer::~index_buffer() {
        if (id) glDeleteBuffers(1, &id);
    }

    void index_buffer::set_data(const void *data, unsigned int size) const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }

    vertex_array::vertex_array() {
        glCreateVertexArrays(1, &id);
    }

    vertex_array::vertex_array(vertex_array &&other) noexcept {
        id = other.id;
        other.id = 0;
    }

    vertex_array &vertex_array::operator=(vertex_array &&other) noexcept {
        if (id) glDeleteVertexArrays(1, &id);
        id = other.id;
        other.id = 0;
        return *this;
    }

    vertex_array::~vertex_array() {
        if (id) glDeleteVertexArrays(1, &id);
    }

    void vertex_array::set_vertex_buffer(vertex_buffer &&vertex_buffer) {
        glBindVertexArray(id);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id);
        vertexBuffer = std::move(vertex_buffer);
    }

    void vertex_array::set_index_buffer(index_buffer &&index_buffer) {
        glBindVertexArray(id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer.id);
        indexBuffer = std::move(index_buffer);
    }

    unsigned int vertex_array::add_attribute(unsigned int index, vertex_array::attribute_type type, int count,
                                             unsigned int offset) {
        glBindVertexArray(id);
        glEnableVertexAttribArray(index);
        switch (type) {
            case attribute_type::float_type:
                glVertexAttribPointer(index, count, GL_FLOAT, GL_FALSE, 0, (void *) (intptr_t) offset);
                return offset + sizeof(float) * count;
            case attribute_type::int_type:
                glVertexAttribIPointer(index, count, GL_UNSIGNED_INT, 0, (void *) (intptr_t) offset);
                return offset + sizeof(int) * count;
            case attribute_type::byte_type:
                glVertexAttribIPointer(index, count, GL_UNSIGNED_BYTE, 0, (void *) (intptr_t) offset);
                return offset + sizeof(char) * count;
            default:
                throw std::runtime_error("invalid attribute type");
        }
    }

    void vertex_array::use() const {
        glBindVertexArray(id);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.id);
    }

    GLenum to_gl_enum(DrawMode mode) {
        switch (mode) {
            case DrawMode::triangles:
                return GL_TRIANGLES;
            case DrawMode::triangle_strip:
                return GL_TRIANGLE_STRIP;
            case DrawMode::triangle_fan:
                return GL_TRIANGLE_FAN;
            case DrawMode::lines:
                return GL_LINES;
            case DrawMode::line_strip:
                return GL_LINE_STRIP;
            case DrawMode::line_loop:
                return GL_LINE_LOOP;
            case DrawMode::points:
                return GL_POINTS;
            default:
                throw std::runtime_error("invalid draw mode");
        }
    }

    void lineWidth(float width) {
        glLineWidth(width);
    }

    void draw(const vertex_array &vertex_array, const shader &shader, const uniform_buffer &uniform_buffer, int count,
              int offset, DrawMode mode) {
        vertex_array.use();
        shader.use();
        for (const auto &uniform: uniform_buffer) {
            shader.set_uniform(uniform.name, uniform.value);
        }
        glDrawElements(to_gl_enum(mode), count, GL_UNSIGNED_INT, (void *) (intptr_t) offset);
    }

    void clear(const cr::math::cvector<float, 4> &color) {
        glClearColor(color[0], color[1], color[2], color[3]);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    cr::math::square_matrix<float, 3> opengl_window_to_pixel(int width, int height) {
        return cr::math::translate_matrix<float>(-1, 1) *
               cr::math::scale_matrix<float>(cr::math::with_translation, 2.0f / width, -2.0f / height);
    }

}