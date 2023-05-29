
#pragma once

#include "matrix.h"

namespace cr::math {

struct with_translation_t {
};
constexpr with_translation_t with_translation{};

template<typename T>
    requires std::is_floating_point_v<T>
square_matrix<T, 2> rotation_matrix(T angle) {
    return {std::cos(angle), -std::sin(angle), std::sin(angle), std::cos(angle)};
}

template<typename T>
    requires std::is_floating_point_v<T>
square_matrix<T, 3> rotation_matrix(with_translation_t, T angle) {
    return {std::cos(angle), -std::sin(angle), 0, std::sin(angle), std::cos(angle), 0, 0, 0, 1};
}

template<typename T>
    requires std::is_floating_point_v<T>
square_matrix<T, 3> rotation_matrix_at(T angle, T x, T y) {
    return {std::cos(angle), -std::sin(angle), x - x * std::cos(angle) + y * std::sin(angle),
            std::sin(angle), std::cos(angle), y - x * std::sin(angle) - y * std::cos(angle),
            0, 0, 1};
}

template<typename T>
    requires std::is_floating_point_v<T>
square_matrix<T, 3> rotation_matrix_xy(T angle) {
    return {std::cos(angle), -std::sin(angle), 0, std::sin(angle), std::cos(angle), 0, 0, 0, 1};
}

template<typename T>
    requires std::is_floating_point_v<T>
square_matrix<T, 4> rotation_matrix_xy(with_translation_t, T angle) {
    return {std::cos(angle), -std::sin(angle), 0, 0, std::sin(angle), std::cos(angle), 0, 0, 0, 0, 1, 0, 0, 0, 0,
            1};
}

template<typename T>
    requires std::is_floating_point_v<T>
square_matrix<T, 3> rotation_matrix_xz(T angle) {
    return {std::cos(angle), 0, -std::sin(angle), 0, 1, 0, std::sin(angle), 0, std::cos(angle)};
}

template<typename T>
    requires std::is_floating_point_v<T>
square_matrix<T, 4> rotation_matrix_xz(with_translation_t, T angle) {
    return {std::cos(angle), 0, -std::sin(angle), 0, 0, 1, 0, 0, std::sin(angle), 0, std::cos(angle), 0, 0, 0, 0,
            1};
}

template<typename T>
    requires std::is_floating_point_v<T>
square_matrix<T, 3> rotation_matrix_yz(T angle) {
    return {1, 0, 0, 0, std::cos(angle), -std::sin(angle), 0, std::sin(angle), std::cos(angle)};
}

template<typename T>
    requires std::is_floating_point_v<T>
square_matrix<T, 4> rotation_matrix_yz(with_translation_t, T angle) {
    return {1, 0, 0, 0, 0, std::cos(angle), -std::sin(angle), 0, 0, std::sin(angle), std::cos(angle), 0, 0, 0, 0,
            1};
}

template<typename T, typename... Args>
    requires std::is_floating_point_v<T>
square_matrix<T, sizeof...(Args)> scale_matrix(Args... args) {
    square_matrix<T, sizeof...(Args)> matrix{};
    auto values = {args...};
    auto it = values.begin();
    for (size_t i = 0; i < sizeof...(Args); ++i) {
        matrix[i][i] = *it;
        ++it;
    }
    return matrix;
}

template<typename T, typename... Args>
    requires std::is_floating_point_v<T>
square_matrix<T, sizeof...(Args) + 1> scale_matrix(with_translation_t, Args... args) {
    square_matrix<T, sizeof...(Args) + 1> matrix{};
    auto values = {args...};
    auto it = values.begin();
    for (size_t i = 0; i < sizeof...(Args); ++i) {
        matrix[i][i] = *it;
        ++it;
    }
    matrix[sizeof...(Args)][sizeof...(Args)] = 1;
    return matrix;
}

template<typename T, typename... Args>
    requires std::is_floating_point_v<T>
square_matrix<T, sizeof...(Args) + 1> translate_matrix(Args... args) {
    matrix matrix = identity<T, sizeof...(Args) + 1>();
    auto values = {args...};
    auto it = values.begin();
    for (size_t i = 0; i < sizeof...(Args); ++i) {
        matrix[i][sizeof...(Args)] = *it;
        ++it;
    }
    return matrix;
}

template<typename vector>
    requires(std::is_floating_point_v<typename vector::Type> && vector_type<vector>)
square_matrix<typename vector::Type, vector_size<vector>() + 1> translate_matrix(const vector& position) {
    matrix matrix = identity<typename vector::Type, vector_size<vector>() + 1>();
    for (size_t i = 0; i < vector_size<vector>(); ++i) {
        matrix[i][vector_size<vector>()] = position[i];
    }
    return matrix;
}

template<typename vector1, typename vector2>
    requires(std::is_floating_point_v<typename vector1::Type> && vector_type<vector1> &&
             std::is_same_v<typename vector1::Type, typename vector2::Type> &&
             vector_size<vector1>() == vector_size<vector2>())
square_matrix<typename vector1::Type, vector_size<vector1>() + 1> scale_matrix_at(const vector1& scale, const vector2& position) {
    return translate_matrix<>(position) * scale_matrix<>(scale) * translate_matrix<>(-position);
}

}// namespace cr::math