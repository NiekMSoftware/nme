#pragma once

namespace nme::math {

template<typename T, usize N, usize M>
struct Matrix;  // fwd

template<typename T, usize N, usize M>
struct MatrixConstants {
    static const Matrix<T, N, M> zero;
};

template<typename T>
    requires is_floating<T>
struct Matrix<T, 2, 2> {
    static const Matrix<T, 2, 2> zero;
    static const Matrix<T, 2, 2> identity;
};

template<typename T>
    requires is_floating<T>
struct Matrix<T, 3, 3> {
    static const Matrix<T, 3, 3> zero;
    static const Matrix<T, 3, 3> identity;
};

template<typename T>
    requires is_floating<T>
struct Matrix<T, 4, 4> {
    static const Matrix<T, 4, 4> zero;
    static const Matrix<T, 4, 4> identity;
};

}  // namespace nme::math