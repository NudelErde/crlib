//
// Created by nudelerde on 20.05.23.
//

#include "crmath/matrix.h"
#include <iostream>

int main() {
    using namespace cr::math;
    using namespace std::complex_literals;
    matrix m1 = identity<float, 2>();
    matrix m2 = identity<float, 2>();

    std::cout << "M1:            " << m1 << std::endl;
    std::cout << "M2:            " << m2 << std::endl;

    std::cout << "M1 + M2:       " << m1 + m2 << std::endl;
    std::cout << "M1 - M2:       " << m1 - m2 << std::endl;
    std::cout << "M1 * M2:       " << m1 * m2 << std::endl;
    std::cout << "M1^T * 3 + M2: " << (m1.transposed() * 3) + m2 << std::endl;
    matrix<float, 2, 2> m3{1.0f, 2.0f, 3.0f, 4.0f};
    std::cout << "M3:            " << m3 << std::endl;
    std::cout << "M3^T:          " << m3.transposed() << std::endl;
    std::cout << "M3.row(0):     " << m3.row_vector(0) << std::endl;
    std::cout << "M3.col(0):     " << m3.column_vector(0) << std::endl;

    std::cout << "M3[0][0]:      " << m3[0][0] << std::endl;
    std::cout << "M3[0][1]:      " << m3[0][1] << std::endl;
    std::cout << "M3[1][0]:      " << m3[1][0] << std::endl;
    std::cout << "M3[1][1]:      " << m3[1][1] << std::endl;

    matrix m4 = identity<float, 7>();
    std::cout << "det(I):        " << determinant(m4) << std::endl;

    rvector<float, 2> v1{1.0f, 2.0f};
    rvector<float, 2> v2{3.0f, 4.0f};

    std::cout << "V1:            " << v1 << std::endl;
    std::cout << "V2:            " << v2 << std::endl;

    std::cout << "V1 + V2:       " << v1 + v2 << std::endl;
    std::cout << "V1 - V2:       " << v1 - v2 << std::endl;
    std::cout << "V1 * V2^T:     " << v1 * v2.transposed() << std::endl;
    std::cout << "V1^T * V2:     " << v1.transposed() * v2 << std::endl;

    matrix m5 = identity<std::complex<double>, 2>() * (2.0 + 1.0i);
    std::cout << "M5:            " << m5 << std::endl;

    matrix<double, 2, 2> m6{5, 2, 7, 9};
    std::cout << "M6:            " << m6 << std::endl;
    std::cout << "det(M6):       " << determinant(m6) << std::endl;
    std::cout << "M6^-1:         " << inverse(m6) << std::endl;
    std::cout << "M6 * M6^-1:    " << m6 * inverse(m6).value_or(matrix<double, 2, 2>{0, 0, 0, 0}) << std::endl;

    matrix<double, 2, 2> m7{1, 2, 3, 4};
    std::cout << "M7:            " << m7 << std::endl;
    std::cout << "adj(M7):       " << adjugate(m7) << std::endl;

    cvector<double, 2> v3 = m7.column_vector(2);

    std::cout << "V3:            " << v3 << std::endl;
}