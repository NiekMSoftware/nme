#pragma once

#include <type_traits>

#include "./vec.hpp"
#include "nme/platform/types.h"

namespace nme::math {

// --------------------------------------------
//                   Storage
// --------------------------------------------

// Generic fallback (column-major: element (r,c) lives at data[c * Rows + r])
template<typename T, u32 Rows, u32 Cols> struct Matrix { T data[Rows * Cols]; };

#define NME_MAT2_MEMBERS(T)                                                    \
    union {                                                                    \
        T data[4];                                                             \
        Vector<T, 2> col[2];                                                   \
    };                                                                         \
    Matrix() = default;                                                        \
    constexpr Matrix(const Vector<T, 2>& c0, const Vector<T, 2>& c1)           \
        : col{c0, c1} {}                                                       \
    constexpr explicit Matrix(T s)                                             \
        : col{Vector<T, 2>(s, T(0)),                                           \
          Vector<T, 2>(T(0), s)} {}                                            \
    template<typename U, std::enable_if_t<!std::is_same_v<U, T>, int> = 0>     \
    constexpr explicit Matrix(const Matrix<U, 2, 2>& m)                        \
        : col{Vector<T, 2>(m.col[0]), Vector<T, 2>(m.col[1])} {}               \
    constexpr Vector<T, 2>&       operator[](u32 c)       { return col[c]; }   \
    constexpr const Vector<T, 2>& operator[](u32 c) const { return col[c]; }   \
    constexpr T&       operator()(u32 r, u32 c)       { return data[c*2 + r]; }\
    constexpr const T& operator()(u32 r, u32 c) const { return data[c*2 + r]; }

#define NME_MAT3_MEMBERS(T)                                                    \
    union {                                                                    \
        T data[9];                                                             \
        Vector<T, 3> col[3];                                                   \
    };                                                                         \
    Matrix() = default;                                                        \
    constexpr Matrix(const Vector<T, 3>& c0, const Vector<T, 3>& c1,           \
                     const Vector<T, 3>& c2)                                   \
        : col{c0, c1, c2} {}                                                   \
    constexpr explicit Matrix(T s)                                             \
        : col{Vector<T, 3>(s, T(0), T(0)),                                     \
              Vector<T, 3>(T(0), s, T(0)),                                     \
              Vector<T, 3>(T(0), T(0), s)} {}                                  \
    template<typename U, std::enable_if_t<!std::is_same_v<U, T>, int> = 0>     \
    constexpr explicit Matrix(const Matrix<U, 3, 3>& m)                        \
        : col{Vector<T, 3>(m.col[0]), Vector<T, 3>(m.col[1]),                  \
              Vector<T, 3>(m.col[2])} {}                                       \
    constexpr Vector<T, 3>&       operator[](u32 c)       { return col[c]; }   \
    constexpr const Vector<T, 3>& operator[](u32 c) const { return col[c]; }   \
    constexpr T&       operator()(u32 r, u32 c)       { return data[c*3 + r]; }\
    constexpr const T& operator()(u32 r, u32 c) const { return data[c*3 + r]; }

#define NME_MAT4_MEMBERS(T)                                                    \
    union {                                                                    \
        T data[16];                                                            \
        Vector<T, 4> col[4];                                                   \
    };                                                                         \
    Matrix() = default;                                                        \
    constexpr Matrix(const Vector<T, 4>& c0, const Vector<T, 4>& c1,           \
                     const Vector<T, 4>& c2, const Vector<T, 4>& c3)           \
        : col{c0, c1, c2, c3} {}                                               \
    constexpr explicit Matrix(T s)                                             \
        : col{Vector<T, 4>(s, T(0), T(0), T(0)),                               \
              Vector<T, 4>(T(0), s, T(0), T(0)),                               \
              Vector<T, 4>(T(0), T(0), s, T(0)),                               \
              Vector<T, 4>(T(0), T(0), T(0), s)} {}                            \
    template<typename U, std::enable_if_t<!std::is_same_v<U, T>, int> = 0>     \
    constexpr explicit Matrix(const Matrix<U, 4, 4>& m)                        \
        : col{Vector<T, 4>(m.col[0]), Vector<T, 4>(m.col[1]),                  \
              Vector<T, 4>(m.col[2]), Vector<T, 4>(m.col[3])} {}               \
    constexpr Vector<T, 4>&       operator[](u32 c)       { return col[c]; }   \
    constexpr const Vector<T, 4>& operator[](u32 c) const { return col[c]; }   \
    constexpr T&       operator()(u32 r, u32 c)       { return data[c*4 + r]; }\
    constexpr const T& operator()(u32 r, u32 c) const { return data[c*4 + r]; }

// ---------- Mat2x2 ----------
template <> struct Matrix<f32, 2, 2> { NME_MAT2_MEMBERS(f32); };
template <> struct Matrix<f64, 2, 2> { NME_MAT2_MEMBERS(f64); };
template <> struct Matrix<i32, 2, 2> { NME_MAT2_MEMBERS(i32); };

// ---------- Mat3x3 ----------
template <> struct Matrix<f32, 3, 3> { NME_MAT3_MEMBERS(f32); };
template <> struct Matrix<f64, 3, 3> { NME_MAT3_MEMBERS(f64); };
template <> struct Matrix<i32, 3, 3> { NME_MAT3_MEMBERS(i32); };

// ---------- Mat4x4 ----------
template <> struct alignas(16) Matrix<f32, 4, 4> { NME_MAT4_MEMBERS(f32); };
template <> struct alignas(16) Matrix<i32, 4, 4> { NME_MAT4_MEMBERS(i32); };
template <> struct             Matrix<f64, 4, 4> { NME_MAT4_MEMBERS(f64); };

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

// Lock the layout so it can't drift silently across compilers
static_assert(sizeof(Mat2x2f) == 16);
static_assert(sizeof(Mat3x3f) == 36);
static_assert(sizeof(Mat4x4f) == 64 && alignof(Mat4x4f) == 16);
static_assert(sizeof(Mat4x4i) == 64 && alignof(Mat4x4i) == 16);
static_assert(std::is_trivially_copyable_v<Mat4x4f>);
static_assert(std::is_standard_layout_v<Mat4x4f>);

}  // namespace nme::math

#include "details/matrix.inl"
