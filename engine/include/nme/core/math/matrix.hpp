#pragma once

#include "nme/platform/graphics/clip_space.h"
#include "details/matrix_base.hpp"
#include "details/matrix_constants.hpp"
#include "details/matrix_traits.hpp"

namespace nme::math {

template<typename T, usize N, usize M = N>
struct Matrix : MatrixBase<T, N, M>, MatrixConstants<T, N, M> {

    using type       = Matrix<T, N, M>;
    using value_type = T;
    using size_type  = usize;

    static constexpr usize row_count = N;
    static constexpr usize col_count = M;
    using base = MatrixBase<T, N, M>;

    // ---- introspection / raw access ----

    static constexpr usize rows() noexcept;
    static constexpr usize cols() noexcept;
    static constexpr usize size() noexcept;
    [[nodiscard]] constexpr T* data() noexcept;

    // ---- constructors ----

    constexpr Matrix() noexcept;

    template<convertible_to<T> X>
    explicit constexpr Matrix(X s) noexcept;

    template<typename... Args>
        requires(sizeof...(Args) >= 2) && matrix_component_pack<T, Args...>
    constexpr Matrix(const Args&... args) noexcept;

    template<convertible_to<T> U, usize K, usize L>
    constexpr Matrix(const Matrix<U, K, L>& other) noexcept;

    template<convertible_to<T> U, usize K, usize L>
    constexpr Matrix<T, N, M>& operator=(const Matrix<U, K, L>& rhs) noexcept;

    // ---- row access: m[i] ----

    constexpr Vector<T, M>& operator[](usize i) noexcept;
    constexpr const Vector<T, M>& operator[](usize i) const noexcept;

    // ---- element access: m(i, j) ----

    constexpr T& operator()(usize i, usize j) noexcept;
    constexpr const T& operator()(usize i, usize j) const noexcept;
};

// --------------------------------------------------------------------------
// Aliases
// --------------------------------------------------------------------------

using Matrix2f = Matrix<f32, 2, 2>;
using Matrix2d = Matrix<f64, 2, 2>;
using Matrix2i = Matrix<i32, 2, 2>;
using Matrix2u = Matrix<u32, 2, 2>;

using Matrix3f = Matrix<f32, 3, 3>;
using Matrix3d = Matrix<f64, 3, 3>;
using Matrix3i = Matrix<i32, 3, 3>;
using Matrix3u = Matrix<u32, 3, 3>;

using Matrix4f = Matrix<f32, 4, 4>;
using Matrix4d = Matrix<f64, 4, 4>;
using Matrix4i = Matrix<i32, 4, 4>;
using Matrix4u = Matrix<u32, 4, 4>;

// Also support 4×3 (3D transforms with scale) and 4×2, 2×3, etc.
using Matrix2x3f = Matrix<f32, 2, 3>;
using Matrix3x2f = Matrix<f32, 3, 2>;
using Matrix4x3f = Matrix<f32, 4, 3>;
using Matrix3x4f = Matrix<f32, 3, 4>;
using Matrix2x4f = Matrix<f32, 2, 4>;
using Matrix4x2f = Matrix<f32, 4, 2>;

using mat2  = Matrix<f32, 2, 2>;
using mat3  = Matrix<f32, 3, 3>;
using mat4  = Matrix<f32, 4, 4>;

using mat2x3 = Matrix<f32, 2, 3>;
using mat3x2 = Matrix<f32, 3, 2>;
using mat3x4 = Matrix<f32, 3, 4>;
using mat4x3 = Matrix<f32, 4, 3>;

using dmat2 = Matrix<f64, 2, 2>;
using dmat3 = Matrix<f64, 3, 3>;
using dmat4 = Matrix<f64, 4, 4>;

using imat2 = Matrix<i32, 2, 2>;
using imat3 = Matrix<i32, 3, 3>;
using imat4 = Matrix<i32, 4, 4>;

using umat2 = Matrix<u32, 2, 2>;
using umat3 = Matrix<u32, 3, 3>;
using umat4 = Matrix<u32, 4, 4>;

}  // namespace nme::math

// --------------------------------------------------------------------------
// Implementation
// --------------------------------------------------------------------------

// TODO: fill in inline headers
#include "details/matrix_ctor.inl"
