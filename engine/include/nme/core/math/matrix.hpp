#pragma once

#include "details/matrix_base.hpp"

namespace nme::math {

template<typename T, usize N, usize M = N>
struct Matrix : MatrixBase<T, N, M> {
    using value_type = T;
    using size_type  = usize;

    using base = MatrixBase<T, N, M>;
};

// ---------- More accessible matrix type aliases ----------

// --- f32 matrices ---
typedef Matrix<f32, 2, 2> Mat2x2f;   // Regular Matrix2 using default floating-point precision
typedef Matrix<f32, 3, 3> Mat3x3f;   // Regular Matrix3 using default floating-point precision
typedef Matrix<f32, 4, 4> Mat4x4f;   // Regular Matrix4 using default floating-point precision

// --- f64 matrices ---
typedef Matrix<f64, 2, 2> Mat2x2d;   // Matrix2 using a double as floating-point precision
typedef Matrix<f64, 3, 3> Mat3x3d;   // Matrix3 using a double as floating-point precision
typedef Matrix<f64, 4, 4> Mat4x4d;   // Matrix4 using a double as floating-point precision

// --- i32 matrices ---
typedef Matrix<i32, 2, 2> Mat2x2i;   // Matrix2 using a signed integer as precision
typedef Matrix<i32, 3, 3> Mat3x3i;   // Matrix3 using a signed integer as precision
typedef Matrix<i32, 4, 4> Mat4x4i;   // Matrix4 using a signed integer as precision

}  // namespace nme::math

// TODO: fill in inline headers
