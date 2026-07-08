#pragma once

namespace nme::math {

template<typename T, usize N, usize M>
struct Matrix;  // fwd

template<typename T, usize N, usize M>
struct MatrixConstants {
    static const Matrix<T, N, M> zero;
};

// Square floating matrices also get `identity`.
template<typename T, usize N>
    requires is_floating<T>
struct MatrixConstants<T, N, N> {
    static const Matrix<T, N, N> zero;
    static const Matrix<T, N, N> identity;
};

}  // namespace nme::math