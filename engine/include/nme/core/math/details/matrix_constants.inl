#pragma once

namespace nme::math {

template<typename T, usize N, usize M>
inline const Matrix<T, N, M> MatrixConstants<T, N, M>::zero{};

template<typename T, usize N>
requires is_floating<T>
inline const Matrix<T, N, N> MatrixConstants<T, N, N>::zero{};

// identity must set the diagonal to 1; the single-scalar ctor only writes the
// first N elements, so build it explicitly.
template<typename T, usize N>
requires is_floating<T>
inline const Matrix<T, N, N> MatrixConstants<T, N, N>::identity = [] {
    Matrix<T, N, N> m{};
    for (usize i = 0; i < N; ++i)
        m(i, i) = T(1);
    return m;
}();

}  // namespace nme::math
