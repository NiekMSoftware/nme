#pragma once

#include <cmath>

#include "details/concepts.hpp"
#include "details/vec_base.hpp"

namespace nme::math {

// --------------------------------------------
//                   Storage
// --------------------------------------------

template<typename T, usize N>
struct Vector : VectorBase<T, N> {

    /** @brief The type of this Vector. */
    using type = Vector<T, N>;

    /** @brief The Vector value type. */
    using value_type = T;

    /** @brief The Vector size type. */
    using size_type = usize;

    /** @brief The number of components of this Vector. */
    static constexpr usize comp = N;

    /** @brief Vector base class. */
    using base = VectorBase<T, N>;

    static const Vector<T, N> zero;
    static const Vector<T, N> one;
    static const Vector<T, N> x;
    static const Vector<T, N> y;
    static const Vector<T, N> z;
    static const Vector<T, N> w;

    static constexpr usize size() noexcept;

    constexpr T* data() noexcept;
    [[nodiscard]] constexpr const T* data() const noexcept;

    constexpr Vector() noexcept;

    template<convertible_to<T> X>
    explicit constexpr Vector(X s) noexcept;

    template<convertible_to<T> X, convertible_to<T> Y>
    constexpr Vector(X x_, Y y_) noexcept;

    template<convertible_to<T> X, convertible_to<T> Y, convertible_to<T> Z>
    constexpr Vector(X x_, Y y_, Z z_) noexcept;

    template<convertible_to<T> X, convertible_to<T> Y, convertible_to<T> Z, convertible_to<T> W>
    constexpr Vector(X x_, Y y_, Z z_, W w_) noexcept;

    // TODO: construct a vector from a span

    template<convertible_to<T> U, usize M, convertible_to<T>... Args>
    explicit constexpr Vector(const Vector<U, M>& copy, const Args&... args) noexcept;

    template<convertible_to<T> X, convertible_to<T> U, usize M>
    constexpr Vector(X x_, const Vector<U, M>& copy);

    template<convertible_to<T> X, convertible_to<T> Y, convertible_to<T> U, usize M>
    constexpr Vector(X x_, Y y_, const Vector<U, M>& copy);

    template<convertible_to<T> X, convertible_to<T> Y, convertible_to<T> Z, convertible_to<T> U, usize M>
    constexpr Vector(X x_, Y y_, Z z_, const Vector<U, M>& copy);

    template<convertible_to<T> X, convertible_to<T> Y, convertible_to<T> Z, convertible_to<T> W, convertible_to<T> U, usize M>
    constexpr Vector(X x_, Y y_, Z z_, W w_, const Vector<U, M>& copy);
};

// ---------- More accessible Vector Types ----------

using Vector2f = Vector<f32, 2>;
using Vector2d = Vector<f64, 2>;
using Vector2i = Vector<i32, 2>;
using Vector2u = Vector<u32, 2>;

using Vector3f = Vector<f32, 3>;
using Vector3d = Vector<f64, 3>;
using Vector3i = Vector<i32, 3>;
using Vector3u = Vector<u32, 3>;

using Vector4f = Vector<f32, 4>;
using Vector4d = Vector<f64, 4>;
using Vector4i = Vector<i32, 4>;
using Vector4u = Vector<u32, 4>;

using vec2 = Vector<f32, 2>;
using vec3 = Vector<f32, 3>;
using vec4 = Vector<f32, 4>;

using float2 = Vector<f32, 2>;
using float3 = Vector<f32, 3>;
using float4 = Vector<f32, 4>;

using double2 = Vector<f64, 2>;
using double3 = Vector<f64, 3>;
using double4 = Vector<f64, 4>;

using int2 = Vector<i32, 2>;
using int3 = Vector<i32, 3>;
using int4 = Vector<i32, 4>;

using uint2 = Vector<u32, 2>;
using uint3 = Vector<u32, 3>;
using uint4 = Vector<u32, 4>;

using usize2 = Vector<usize, 2>;
using usize3 = Vector<usize, 3>;
using usize4 = Vector<usize, 4>;

}  // namespace nme::math

#include "details/vec.inl"

// EOF