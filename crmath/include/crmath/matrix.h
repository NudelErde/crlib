//
// Created by nudelerde on 20.05.23.
//

#pragma once

#include <cstdlib>
#include <type_traits>
#include <ostream>
#include <complex>
#include <optional>

namespace cr::math {
    template<typename Mat>
    concept matrix_type = requires {
        typename Mat::Type;
        { Mat::rows() } -> std::convertible_to<size_t>;
        { Mat::columns() } -> std::convertible_to<size_t>;
    };

    template<typename Mat1, typename Mat2>
    concept same_size_matrix =
    Mat1::rows() == Mat2::rows() &&
    Mat1::columns() == Mat2::columns() &&
    std::is_convertible_v<typename Mat1::Type, typename Mat2::Type> &&
    std::is_convertible_v<typename Mat2::Type, typename Mat1::Type>;

    template<typename Mat1, typename Mat2>
    concept multipliable_matrix =
    Mat1::columns() == Mat2::rows() &&
    std::is_convertible_v<typename Mat1::Type, typename Mat2::Type> &&
    std::is_convertible_v<typename Mat2::Type, typename Mat1::Type>;

    template<typename Mat>
    concept square_matrix_concept = Mat::rows() == Mat::columns();

    template<typename T, size_t N, size_t M>
    struct matrix;

    template<typename MatrixType>
    struct matrix_transposed_view;

    template<typename MatType>
    using matrix_modifiable = matrix<typename MatType::Type, MatType::rows(), MatType::columns()>;

    template<typename MatTypeLeft, typename MatTypeRight>
    using matrix_modifiable_multiply = matrix<typename MatTypeLeft::Type, MatTypeLeft::rows(), MatTypeRight::columns()>;

    template<typename MatType>
    concept modifiable_matrix = std::is_same_v<MatType, matrix_modifiable<MatType>>;

    template<typename CVecType>
    concept cvector_type = requires {
        typename CVecType::Type;
        { CVecType::rows() } -> std::convertible_to<size_t>;
    };

    template<typename RVecType>
    concept rvector_type = requires {
        typename RVecType::Type;
        { RVecType::columns() } -> std::convertible_to<size_t>;
    };

    template<typename VecType>
    concept vector_type = cvector_type<VecType> || rvector_type<VecType>;

    template<cvector_type Vec>
    constexpr size_t vector_size() { return Vec::rows(); }

    template<rvector_type Vec>
    constexpr size_t vector_size() { return Vec::columns(); }

    auto rk4(auto x_curr, auto dt, auto f) {
        auto k1 = f(x_curr);
        auto k2 = f(x_curr + dt * k1 / 2);
        auto k3 = f(x_curr + dt * k2 / 2);
        auto k4 = f(x_curr + dt * k3);
        return x_curr + dt * (k1 + 2 * k2 + 2 * k3 + k4) / 6;
    }

    template<modifiable_matrix MatType>
    [[nodiscard]] constexpr MatType &operator+=(MatType &lhs, const MatType &rhs) {
        for (size_t i = 0; i < MatType::rows(); i++) {
            for (size_t j = 0; j < MatType::columns(); j++) {
                lhs.access(i, j) += rhs.access(i, j);
            }
        }
        return lhs;
    }

    template<modifiable_matrix MatType>
    [[nodiscard]] constexpr MatType &operator-=(MatType &lhs, const MatType &rhs) {
        for (size_t i = 0; i < MatType::rows(); i++) {
            for (size_t j = 0; j < MatType::columns(); j++) {
                lhs.access(i, j) -= rhs.access(i, j);
            }
        }
        return lhs;
    }

    template<modifiable_matrix MatType, typename Scalar>
    requires std::is_convertible_v<Scalar, typename MatType::Type>
    [[nodiscard]] constexpr MatType &operator*=(MatType &lhs, const Scalar &scalar) {
        for (size_t i = 0; i < MatType::rows(); i++) {
            for (size_t j = 0; j < MatType::columns(); j++) {
                lhs.access(i, j) *= scalar;
            }
        }
        return lhs;
    }

    template<modifiable_matrix MatType, typename Scalar>
    requires std::is_convertible_v<Scalar, typename MatType::Type>
    [[nodiscard]] constexpr MatType &operator/=(MatType &lhs, const Scalar &scalar) {
        for (size_t i = 0; i < MatType::rows(); i++) {
            for (size_t j = 0; j < MatType::columns(); j++) {
                lhs.access(i, j) /= scalar;
            }
        }
        return lhs;
    }

    template<typename MatTypeLeft, typename MatTypeRight>
    requires same_size_matrix<MatTypeLeft, MatTypeRight>
    constexpr matrix_modifiable<MatTypeLeft> operator+(const MatTypeLeft &lhs, const MatTypeRight &rhs) {
        matrix_modifiable<MatTypeLeft> res;
        for (size_t i = 0; i < MatTypeLeft::rows(); i++) {
            for (size_t j = 0; j < MatTypeLeft::columns(); j++) {
                res.access(i, j) = lhs.access(i, j) + rhs.access(i, j);
            }
        }
        return res;
    }

    template<typename MatTypeLeft, typename MatTypeRight>
    requires same_size_matrix<MatTypeLeft, MatTypeRight>
    constexpr matrix_modifiable<MatTypeLeft> operator-(const MatTypeLeft &lhs, const MatTypeRight &rhs) {
        matrix_modifiable<MatTypeLeft> res;
        for (size_t i = 0; i < MatTypeLeft::rows(); i++) {
            for (size_t j = 0; j < MatTypeLeft::columns(); j++) {
                res.access(i, j) = lhs.access(i, j) - rhs.access(i, j);
            }
        }
        return res;
    }

    template<typename MatType, typename Scalar>
    requires std::is_convertible_v<Scalar, typename MatType::Type>
    constexpr matrix_modifiable<MatType> operator*(const MatType &lhs, const Scalar &scalar) {
        matrix_modifiable<MatType> res;
        for (size_t i = 0; i < MatType::rows(); i++) {
            for (size_t j = 0; j < MatType::columns(); j++) {
                res.access(i, j) = lhs.access(i, j) * scalar;
            }
        }
        return res;
    }

    template<typename MatType, typename Scalar>
    requires std::is_convertible_v<Scalar, typename MatType::Type>
    constexpr matrix_modifiable<MatType> operator*(const Scalar &scalar, const MatType &rhs) {
        matrix_modifiable<MatType> res;
        for (size_t i = 0; i < MatType::rows(); i++) {
            for (size_t j = 0; j < MatType::columns(); j++) {
                res.access(i, j) = rhs.access(i, j) * scalar;
            }
        }
        return res;
    }

    template<typename MatType, typename Scalar>
    requires std::is_convertible_v<Scalar, typename MatType::Type>
    constexpr matrix_modifiable<MatType> operator/(const MatType &lhs, const Scalar &scalar) {
        matrix_modifiable<MatType> res;
        for (size_t i = 0; i < MatType::rows(); i++) {
            for (size_t j = 0; j < MatType::columns(); j++) {
                res.access(i, j) = lhs.access(i, j) / scalar;
            }
        }
        return res;
    }

    template<typename MatTypeLeft, typename MatTypeRight>
    requires multipliable_matrix<MatTypeLeft, MatTypeRight>
    constexpr matrix_modifiable_multiply<MatTypeLeft, MatTypeRight>
    operator*(const MatTypeLeft &lhs, const MatTypeRight &rhs) {
        matrix_modifiable_multiply<MatTypeLeft, MatTypeRight> res;
        for (size_t i = 0; i < MatTypeLeft::rows(); i++) {
            for (size_t j = 0; j < MatTypeRight::columns(); j++) {
                res.access(i, j) = 0;
                for (size_t k = 0; k < MatTypeLeft::columns(); k++) {
                    res.access(i, j) += lhs.access(i, k) * rhs.access(k, j);
                }
            }
        }
        return res;
    }

    template<typename MatTypeLeft, typename MatTypeRight>
    requires same_size_matrix<MatTypeLeft, MatTypeRight>
    bool operator==(const MatTypeLeft &lhs, const MatTypeRight &rhs) {
        for (size_t i = 0; i < MatTypeLeft::rows(); i++) {
            for (size_t j = 0; j < MatTypeLeft::columns(); j++) {
                if (lhs.access(i, j) != rhs.access(i, j)) {
                    return false;
                }
            }
        }
        return true;
    }

    template<typename MatTypeLeft, typename MatTypeRight>
    requires same_size_matrix<MatTypeLeft, MatTypeRight>
    bool operator!=(const MatTypeLeft &lhs, const MatTypeRight &rhs) {
        return !(lhs == rhs);
    }

    template<typename MatType>
    requires matrix_type<MatType>
    std::ostream &operator<<(std::ostream &os, const std::optional<MatType> &matrix_opt) {
        if (matrix_opt.has_value()) {
            os << matrix_opt.value();
        } else {
            os << "None";
        }
        return os;
    }

    template<typename MatType>
    requires (matrix_type<MatType> && MatType::rows() == 1 && MatType::columns() == 1)
    std::ostream &operator<<(std::ostream &os, const MatType &matrix) {
        os << '[';
        os << matrix[0];
        os << ']';
        return os;
    }

    template<typename MatType>
    requires (matrix_type<MatType> && MatType::rows() == 1 && MatType::columns() > 1)
    std::ostream &operator<<(std::ostream &os, const MatType &matrix) {
        os << '[';
        for (size_t i = 0; i < MatType::columns(); i++) {
            os << matrix[i];
            if (i != MatType::columns() - 1) {
                os << ", ";
            }
        }
        os << ']';
        return os;
    }

    template<typename MatType>
    requires (matrix_type<MatType> && MatType::rows() > 1 && MatType::columns() == 1)
    std::ostream &operator<<(std::ostream &os, const MatType &matrix) {
        os << '[';
        for (size_t i = 0; i < MatType::rows(); i++) {
            os << matrix[i];
            if (i != MatType::rows() - 1) {
                os << ", ";
            }
        }
        os << "]^T";
        return os;
    }

    template<typename MatrixType>
    requires (matrix_type<MatrixType> && MatrixType::rows() > 1 && MatrixType::columns() > 1)
    std::ostream &operator<<(std::ostream &os, const MatrixType &matrix) {
        os << '[';
        for (size_t i = 0; i < MatrixType::rows(); i++) {
            os << '[';
            for (size_t j = 0; j < MatrixType::columns(); j++) {
                os << matrix[i][j];
                if (j != MatrixType::columns() - 1) {
                    os << ", ";
                }
            }
            os << ']';
            if (i != MatrixType::rows() - 1) {
                os << ", ";
            }
        }
        os << ']';
        return os;
    }

    template<typename MatType>
    requires (MatType::rows() == 1)
    constexpr auto length(const MatType &mat) {
        auto res = 0;
        for (size_t i = 0; i < MatType::columns(); i++) {
            res += mat[i] * mat[i];
        }
        return std::sqrt(res);
    }

    template<typename MatType>
    requires (MatType::columns() == 1)
    constexpr auto length(const MatType &mat) {
        auto res = 0;
        for (size_t i = 0; i < MatType::rows(); i++) {
            res += mat[i] * mat[i];
        }
        return std::sqrt(res);
    }

    template<typename MatType>
    requires square_matrix_concept<MatType>
    constexpr typename MatType::Type determinant(const MatType &mat) {
        if constexpr (MatType::rows() == 1) {
            return mat[0];
        } else if constexpr (MatType::rows() == 2) {
            return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
        } else {
            typename MatType::Type res = 0;
            for (size_t i = 0; i < MatType::rows(); i++) {
                res += mat[0][i] * determinant(mat.minor(0, i));
            }
            return res;
        }
    }

    template<typename MatType>
    constexpr auto transposed(const MatType &mat) {
        return mat.transposed();
    }

    template<typename MatType>
    constexpr auto hermitian(const MatType &mat) {
        MatType res;
        for (size_t i = 0; i < MatType::rows(); i++) {
            for (size_t j = 0; j < MatType::columns(); j++) {
                res.access(i, j) = std::conj(mat.access(j, i));
            }
        }
        return res;
    }

    template<typename T, size_t N>
    constexpr matrix<T, N, N> identity() {
        matrix<T, N, N> res{};
        for (size_t i = 0; i < N; i++) {
            res[i][i] = 1;
        }
        return res;
    }

    template<square_matrix_concept MatType>
    constexpr auto adjugate(const MatType &mat) {
        matrix_modifiable<MatType> res;
        for (size_t i = 0; i < MatType::rows(); i++) {
            for (size_t j = 0; j < MatType::columns(); j++) {
                res.access(j, i) = ((i + j) % 2 == 0 ? 1 : -1) * determinant(mat.minor(i, j));
            }
        }
        return res;
    }

    template<square_matrix_concept MatType>
    constexpr std::optional<matrix_modifiable<MatType>> inverse(const MatType &mat) {
        auto det = determinant(mat);
        if (det == 0) {
            return std::nullopt;
        }
        return adjugate(mat) / det;
    }

    template<typename T, size_t N, size_t M>
    struct matrix {
        static_assert(N > 0 && M > 0, "Matrix dimensions must be positive");
        using Type = T;
        using TransposeRefType = matrix &;

        matrix() = default;

        matrix(const matrix &other) {
            for (size_t i = 0; i < N; i++) {
                for (size_t j = 0; j < M; j++) {
                    access(i, j) = other.access(i, j);
                }
            }
        }

        matrix(matrix &&other) noexcept {
            for (size_t i = 0; i < N; i++) {
                for (size_t j = 0; j < M; j++) {
                    access(i, j) = std::move(other.access(i, j));
                }
            }
        }

        matrix &operator=(const matrix &other) {
            if (this == &other) {
                return *this;
            }
            for (size_t i = 0; i < N; i++) {
                for (size_t j = 0; j < M; j++) {
                    access(i, j) = other.access(i, j);
                }
            }
            return *this;
        }

        matrix &operator=(matrix &&other) noexcept {
            if (this == &other) {
                return *this;
            }
            for (size_t i = 0; i < N; i++) {
                for (size_t j = 0; j < M; j++) {
                    access(i, j) = std::move(other.access(i, j));
                }
            }
            return *this;
        }

        template<typename MatType>
        requires same_size_matrix<MatType, matrix>
        constexpr matrix(const MatType &mat) {
            for (size_t i = 0; i < N; i++) {
                for (size_t j = 0; j < M; j++) {
                    access(i, j) = mat.access(i, j);
                }
            }
        }

        template<typename ...ArgT>
        requires (sizeof...(ArgT) == N * M)
        matrix(ArgT &&...args) : data{static_cast<Type>(args)...} {
        }

        [[nodiscard]] static constexpr size_t rows() {
            return N;
        }

        [[nodiscard]] static constexpr size_t columns() {
            return M;
        }

        Type *raw() {
            return &data[0][0];
        }

        const Type *raw() const {
            return &data[0][0];
        }

        struct row_view {
            row_view(size_t row, matrix &mat) : row(row), mat(mat) {}

            [[nodiscard]] constexpr T &operator[](size_t i) {
                return mat.data[row][i];
            }

            [[nodiscard]] constexpr const T &operator[](size_t i) const {
                return mat.data[row][i];
            }

        private:
            size_t row;
            matrix &mat;
        };

        struct const_row_view {
            const_row_view(size_t row, const matrix &mat) : row(row), mat(mat) {}

            [[nodiscard]] constexpr const T &operator[](size_t i) const {
                return mat.data[row][i];
            }

        private:
            size_t row;
            const matrix &mat;
        };

        [[nodiscard]] constexpr decltype(auto) operator[](size_t i) {
            if constexpr (rows() == 1) {
                return access(0, i);
            } else if constexpr (columns() == 1) {
                return access(i, 0);
            } else {
                return row_view{i, *this};
            }
        }

        [[nodiscard]] constexpr decltype(auto) operator[](size_t i) const {
            if constexpr (rows() == 1) {
                return access(0, i);
            } else if constexpr (columns() == 1) {
                return access(i, 0);
            } else {
                return const_row_view{i, *this};
            }
        }

        [[nodiscard]] constexpr T &access(size_t i, size_t j) {
            return data[i][j];
        }

        [[nodiscard]] constexpr T access(size_t i, size_t j) const {
            return data[i][j];
        }

        template<size_t ROW_COUNT, size_t COLUMN_COUNT, typename MatrixType>
        struct matrix_view {
            explicit matrix_view(MatrixType &mat) : mat(mat) {}

            //using MatrixType = matrix<T, N, M>;
            using Type = T;
            using TransposeRefType = matrix_view;

            [[nodiscard]] static constexpr size_t rows() {
                return ROW_COUNT;
            }

            [[nodiscard]] static constexpr size_t columns() {
                return COLUMN_COUNT;
            }

            template<size_t SUB_ROW_COUNT, size_t SUB_COLUMN_COUNT>
            requires (SUB_ROW_COUNT <= rows() && SUB_COLUMN_COUNT <= columns())
            [[nodiscard]] constexpr auto submatrix(size_t row, size_t column) {
                typename MatrixType::template matrix_view<SUB_ROW_COUNT, SUB_COLUMN_COUNT, MatrixType> res;
                for (size_t i = 0; i < SUB_ROW_COUNT; i++) {
                    res.used_rows[get_row(i + row)] = true;
                }
                for (size_t i = 0; i < SUB_COLUMN_COUNT; i++) {
                    res.used_columns[get_column(i + column)] = true;
                }
                return res;
            }

            template<size_t SUB_ROW_COUNT, size_t SUB_COLUMN_COUNT>
            requires (SUB_ROW_COUNT <= rows() && SUB_COLUMN_COUNT <= columns())
            [[nodiscard]] constexpr auto submatrix(size_t row, size_t column) const {
                typename MatrixType::template matrix_view<SUB_ROW_COUNT, SUB_COLUMN_COUNT, const MatrixType> res;
                for (size_t i = 0; i < SUB_ROW_COUNT; i++) {
                    res.used_rows[get_row(i + row)] = true;
                }
                for (size_t i = 0; i < SUB_COLUMN_COUNT; i++) {
                    res.used_columns[get_column(i + column)] = true;
                }
                return res;
            }

            constexpr auto row_vector(size_t row) {
                return submatrix<1, columns()>(row, 0);
            }

            constexpr auto row_vector(size_t row) const {
                return submatrix<1, columns()>(row, 0);
            }

            constexpr auto column_vector(size_t row) {
                return submatrix<rows(), 1>(0, row);
            }

            constexpr auto column_vector(size_t column) const {
                return submatrix<rows(), 1>(0, column);
            }

            [[nodiscard]] constexpr auto minor(size_t row, size_t column) {
                static_assert(rows() > 1 && columns() > 1, "Matrix must be at least 2x2");
                matrix_view<rows() - 1, columns() - 1, MatrixType> res{mat};
                for (size_t i = 0; i < rows() - 1; i++) {
                    res.used_rows[get_row(i)] = i != row;
                }
                for (size_t i = 0; i < columns() - 1; i++) {
                    res.used_columns[get_column(i)] = i != column;
                }
                return res;
            }

            [[nodiscard]] constexpr auto minor(size_t row, size_t column) const {
                static_assert(rows() > 1 && columns() > 1, "Matrix must be at least 2x2");
                matrix_view<rows() - 1, columns() - 1, const MatrixType> res{mat};
                for (size_t i = 0; i < rows() - 1; i++) {
                    res.used_rows[get_row(i)] = i != row;
                }
                for (size_t i = 0; i < columns() - 1; i++) {
                    res.used_columns[get_column(i)] = i != column;
                }
                return res;
            }

            constexpr matrix_transposed_view<matrix_view> transposed() {
                return {*this};
            }

            constexpr matrix_transposed_view<const matrix_view> transposed() const {
                return {*this};
            }

            struct row_view {
                row_view(size_t row, matrix_view &mat_view) : row(row), mat_view(mat_view) {}

                [[nodiscard]] constexpr T &operator[](size_t i) {
                    return mat_view.mat.data[row][mat_view.get_column(i)];
                }

                [[nodiscard]] constexpr const T &operator[](size_t i) const {
                    return mat_view.mat.data[row][mat_view.get_column(i)];
                }

            private:
                size_t row;
                matrix_view &mat_view;
            };

            struct const_row_view {
                const_row_view(size_t row, const matrix_view &mat_view) : row(row), mat_view(mat_view) {}

                [[nodiscard]] constexpr const T &operator[](size_t i) const {
                    return mat_view.mat.data[row][mat_view.get_column(i)];
                }

            private:
                size_t row;
                const matrix_view &mat_view;
            };

            [[nodiscard]] constexpr decltype(auto) operator[](size_t i) {
                if constexpr (rows() == 1) {
                    return access(0, i);
                } else if constexpr (columns() == 1) {
                    return access(i, 0);
                } else {
                    return row_view{get_row(i), *this};
                }
            }

            [[nodiscard]] constexpr decltype(auto) operator[](size_t i) const {
                if constexpr (rows() == 1) {
                    return access(0, i);
                } else if constexpr (columns() == 1) {
                    return access(i, 0);
                } else {
                    return const_row_view{get_row(i), *this};
                }
            }

            [[nodiscard]] constexpr Type &access(size_t i, size_t j) {
                return mat.access(get_row(i), get_column(j));
            }

            [[nodiscard]] constexpr Type access(size_t i, size_t j) const {
                return mat.access(get_row(i), get_column(j));
            }

            [[nodiscard]] constexpr size_t get_row(size_t i) const {
                for (size_t j = 0; j < N; j++) {
                    if (used_rows[j]) {
                        if (i == 0) {
                            return j;
                        }
                        --i;
                    }
                }
                return 0;
            }

            [[nodiscard]] constexpr size_t get_column(size_t i) const {
                for (size_t j = 0; j < M; j++) {
                    if (used_columns[j]) {
                        if (i == 0) {
                            return j;
                        }
                        --i;
                    }
                }
                return 0;
            }

            bool used_rows[N]{};
            bool used_columns[M]{};
        private:
            MatrixType &mat;
        };

        template<size_t SUB_ROW_COUNT, size_t SUB_COLUMN_COUNT>
        requires (SUB_ROW_COUNT <= rows() && SUB_COLUMN_COUNT <= columns())
        [[nodiscard]] constexpr auto submatrix(size_t row, size_t column) {
            matrix_view<SUB_ROW_COUNT, SUB_COLUMN_COUNT, matrix> res{*this};
            for (size_t i = 0; i < N; i++) {
                res.used_rows[i + row] = true;
            }
            for (size_t i = 0; i < M; i++) {
                res.used_columns[i + column] = true;
            }
            return res;
        }

        template<size_t SUB_ROW_COUNT, size_t SUB_COLUMN_COUNT>
        requires (SUB_ROW_COUNT <= rows() && SUB_COLUMN_COUNT <= columns())
        [[nodiscard]] constexpr auto submatrix(size_t row, size_t column) const {
            matrix_view<SUB_ROW_COUNT, SUB_COLUMN_COUNT, const matrix> res{*this};
            for (size_t i = 0; i < N; i++) {
                res.used_rows[i + row] = true;
            }
            for (size_t i = 0; i < M; i++) {
                res.used_columns[i + column] = true;
            }
            return res;
        }

        constexpr auto row_vector(size_t row) {
            return submatrix<1, columns()>(row, 0);
        }

        constexpr auto row_vector(size_t row) const {
            return submatrix<1, columns()>(row, 0);
        }

        constexpr auto column_vector(size_t column) {
            return submatrix<rows(), 1>(0, column);
        }

        constexpr auto column_vector(size_t column) const {
            return submatrix<rows(), 1>(0, column);
        }

        [[nodiscard]] constexpr auto minor(size_t row, size_t column) {
            static_assert(rows() > 1 && columns() > 1, "Matrix must be at least 2x2");
            matrix_view<rows() - 1, columns() - 1, matrix> res{*this};
            for (size_t i = 0; i < N; i++) {
                res.used_rows[i] = i != row;
            }
            for (size_t i = 0; i < M; i++) {
                res.used_columns[i] = i != column;
            }
            return res;
        }

        [[nodiscard]] constexpr auto minor(size_t row, size_t column) const {
            static_assert(rows() > 1 && columns() > 1, "Matrix must be at least 2x2");
            matrix_view<rows() - 1, columns() - 1, const matrix> res{*this};
            for (size_t i = 0; i < N; i++) {
                res.used_rows[i] = i != row;
            }
            for (size_t i = 0; i < M; i++) {
                res.used_columns[i] = i != column;
            }
            return res;
        }

        constexpr auto transposed() {
            return matrix_transposed_view<matrix>{*this};
        }

        constexpr auto transposed() const {
            return matrix_transposed_view<const matrix>{*this};
        }

    private:
        T data[N][M]{};
    };

    template<typename MatrixType>
    struct matrix_transposed_view {
        using Type = typename MatrixType::Type;

        explicit matrix_transposed_view(typename MatrixType::TransposeRefType &ref) : mat(ref) {}

        [[nodiscard]] static constexpr size_t rows() {
            return MatrixType::columns();
        }

        [[nodiscard]] static constexpr size_t columns() {
            return MatrixType::rows();
        }

        struct row_view {
            row_view(size_t column, const typename MatrixType::TransposeRefType mat) : column(column), mat(mat) {}

            [[nodiscard]] constexpr Type &operator[](size_t i) {
                return mat[i][column];
            }

            [[nodiscard]] constexpr const Type &operator[](size_t i) const {
                return mat[i][column];
            }

        private:
            size_t column;
            typename MatrixType::TransposeRefType &mat;
        };

        struct const_row_view {
            const_row_view(size_t column, const typename MatrixType::TransposeRefType mat) : column(column), mat(mat) {}

            [[nodiscard]] constexpr const Type &operator[](size_t i) const {
                return mat[i][column];
            }

        private:
            size_t column;
            const typename MatrixType::TransposeRefType mat;
        };

        [[nodiscard]] constexpr decltype(auto) operator[](size_t i) {
            if constexpr (rows() == 1) {
                return mat.access(i, 0);
            } else if constexpr (columns() == 1) {
                return mat.access(0, i);
            } else {
                return row_view{i, mat};
            }
        }

        [[nodiscard]] constexpr decltype(auto) operator[](size_t i) const {
            if constexpr (rows() == 1) {
                return mat.access(i, 0);
            } else if constexpr (columns() == 1) {
                return mat.access(0, i);
            } else {
                return const_row_view{i, mat};
            }
        }

        [[nodiscard]] constexpr auto access(size_t i, size_t j) {
            return mat.access(j, i);
        }

        [[nodiscard]] constexpr auto access(size_t i, size_t j) const {
            return mat.access(j, i);
        }

        template<size_t SUB_ROW_COUNT, size_t SUB_COLUMN_COUNT>
        [[nodiscard]] constexpr auto submatrix(size_t row, size_t column) {
            return mat.template submatrix<SUB_COLUMN_COUNT, SUB_ROW_COUNT>(column, row);
        }

        template<size_t SUB_ROW_COUNT, size_t SUB_COLUMN_COUNT>
        [[nodiscard]] constexpr auto submatrix(size_t row, size_t column) const {
            return mat.template submatrix<SUB_COLUMN_COUNT, SUB_ROW_COUNT>(column, row);
        }


        constexpr auto row_vector(size_t row) {
            return submatrix<1, columns()>(row, 0);
        }

        constexpr auto row_vector(size_t row) const {
            return submatrix<1, columns()>(row, 0);
        }

        constexpr auto column_vector(size_t column) {
            return submatrix<rows(), 1>(0, column);
        }

        constexpr auto column_vector(size_t column) const {
            return submatrix<rows(), 1>(0, column);
        }

        [[nodiscard]] constexpr auto minor(size_t row, size_t column) {
            return mat.minor(column, row).transposed();
        }

        constexpr auto transposed() {
            return mat.template submatrix<MatrixType::columns(), MatrixType::rows()>(0, 0);
        }

    private:
        typename MatrixType::TransposeRefType &mat;
    };

    template<typename T, size_t N>
    using square_matrix = matrix<T, N, N>;

    template<typename T, size_t N>
    using row_vector = matrix<T, 1, N>;

    template<typename T, size_t N>
    using rvector = row_vector<T, N>;

    template<typename T, size_t N>
    using column_vector = matrix<T, N, 1>;

    template<typename T, size_t N>
    using cvector = column_vector<T, N>;
}