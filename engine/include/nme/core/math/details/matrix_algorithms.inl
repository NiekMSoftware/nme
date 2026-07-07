#pragma once

namespace nme::math {

template<typename T, usize N, usize M>
struct transpose_impl;  // fwd

// Generic transpose
template<typename T, usize N, usize M>
struct transpose_impl {
    static constexpr Matrix<T, N, M> transpose(const Matrix<T, N, M>& m) noexcept {
        Matrix<T, N, M> r;
        for (usize i = 0; i < N; ++i)
            for (usize j = 0; j < M; ++j)
                r(j, i) = m(i, j);
        return r;
    }
};

// Square matrix transpose
template<typename T, usize N>
struct transpose_impl<T, N, N> {
    static constexpr Matrix<T, N, N> transpose(const Matrix<T, N, N>& m) noexcept {
        Matrix<T, N, N> r = m;
        for (usize i = 0; i < N; ++i)
            for (usize j = 0; j < N; ++j)
                std::swap(r(i, j), r(j, i));
        return r;
    }
};

template<typename T, usize N, usize M>
constexpr Matrix<T, M, N> transpose(const Matrix<T, N, M>& m) noexcept {
    return transpose_impl<T, N, M>::transpose(m);
}

template<typename T, usize N>
struct determinant_impl;

// 1x1 determinant: just the single element
template<typename T>
struct determinant_impl<T, 1> {
    static constexpr T determinant(const Matrix<T, 1, 1>& m) noexcept {
        return m(0, 0);
    }
};

// 2x2 determinant: ad - bc
template<typename T>
struct determinant_impl<T, 2> {
    static constexpr T determinant(const Matrix<T, 2, 2>& m) noexcept {
        return m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0);
    }
};

// 3x3 determinant: Sarrus rule or cofactor expansion
template<typename T>
struct determinant_impl<T, 3> {
    static constexpr T determinant(const Matrix<T, 3, 3>& m) noexcept {
        return m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1))
             - m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0))
             + m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));
    }
};

// 4x4 determinant: cofactor expansion on first row
template<typename T>
struct determinant_impl<T, 4> {
    static constexpr T determinant(const Matrix<T, 4, 4>& m) noexcept {
        const T m00 = m(0, 0), m01 = m(0, 1), m02 = m(0, 2), m03 = m(0, 3);
        const T m10 = m(1, 0), m11 = m(1, 1), m12 = m(1, 2), m13 = m(1, 3);
        const T m20 = m(2, 0), m21 = m(2, 1), m22 = m(2, 2), m23 = m(2, 3);
        const T m30 = m(3, 0), m31 = m(3, 1), m32 = m(3, 2), m33 = m(3, 3);

        const T c00 = m11 * (m22 * m33 - m23 * m32) - m12 * (m21 * m33 - m23 * m31) + m13 * (m21 * m32 - m22 * m31);
        const T c01 = m10 * (m22 * m33 - m23 * m32) - m12 * (m20 * m33 - m23 * m30) + m13 * (m20 * m32 - m22 * m30);
        const T c02 = m10 * (m21 * m33 - m23 * m31) - m11 * (m20 * m33 - m23 * m30) + m13 * (m20 * m32 - m21 * m30);
        const T c03 = m10 * (m21 * m32 - m22 * m31) - m11 * (m20 * m32 - m22 * m30) + m12 * (m20 * m31 - m21 * m30);

        return m00 * c00 - m01 * c01 + m02 * c02 - m03 * c03;
    }
};

// Generic NxN (cofactor expansion on first row). Used for N >= 5.
template<typename T, usize N>
struct determinant_impl {
    static constexpr T determinant(const Matrix<T, N, N>& m) noexcept {
        T det = T(0);
        for (usize j = 0; j < N; ++j) {
            // Compute the (N-1)x(N-1) minor for cofactor(0, j).
            Matrix<T, N - 1, N -1> minor;
            for (usize mi = 1; mi < N; ++mi) {
                usize col_idx = 0;
                for (usize mj = 0; mj < N; ++mj) {
                    if (mj != j) {
                        minor(mi - 1, col_idx++) = m(mi, mj);
                    }
                }
            }
            const T cofactor = (j % 2 == 0 ? T(1) : T(-1)) * determinant_impl<T, N - 1>::determinant(minor);
            det += m(0, j) * cofactor;
        }
        return det;
    }
};

template<typename T, usize N>
constexpr T determinant(const Matrix<T, N, N>& m) noexcept {
    return determinant_impl<T, N>::determinant(m);
}

}  // namespace nme::math