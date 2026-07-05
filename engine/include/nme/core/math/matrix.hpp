#pragma once

#include "nme/platform/types.h"

namespace nme::math {

// Generic fallback
template<typename T, i32 rows, i32 cols>
struct Matrix { T data[rows][cols]; };

template <> struct Matrix<f32, 1, 1> { using type = Matrix<f32, 1, 1>; };
template <> struct Matrix<f32, 2, 2> { using type = Matrix<f32, 2, 2>; };
template <> struct Matrix<f32, 3, 3> { using type = Matrix<f32, 3, 3>; };
template <> struct Matrix<f32, 4, 4> { using type = Matrix<f32, 4, 4>; };

typedef Matrix<f32, 2, 2> Matrix2f;
typedef Matrix<f32, 3, 3> Matrix3f;
typedef Matrix<f32, 4, 4> Matrix4f;

}  // namespace nme::math

#include "details/matrix.inl"
