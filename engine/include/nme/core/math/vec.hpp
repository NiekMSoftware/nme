#pragma once

#include <cmath>

#include "nme/platform/types.h"
#include "details/vec_base.hpp"

namespace nme::math {

namespace detail {

}  // namespace detail

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

    static const Vector<T, N> zero;
    static const Vector<T, N> one;
    static const Vector<T, N> x;
    static const Vector<T, N> y;
    static const Vector<T, N> z;
    static const Vector<T, N> w;

    static constexpr usize size() noexcept;

    constexpr T* data() noexcept;
    constexpr const T* data() const noexcept;

    using base = VectorBase<T, N>;

    constexpr Vector() noexcept;
};

#undef NME_VEC2_MEMBERS
#undef NME_VEC3_MEMBERS
#undef NME_VEC4_MEMBERS

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

// Lock the layout so it can't drift silently across compilers
// static_assert(sizeof(Vector2f) == 8);
// static_assert(sizeof(Vector3f) == 12);
// static_assert(sizeof(Vector4f) == 16 && alignof(Vector4f) == 16);
// static_assert(sizeof(Vector4i) == 16 && alignof(Vector4i) == 16);
// static_assert(std::is_trivially_copyable_v<Vector4f>);
// static_assert(std::is_standard_layout_v<Vector4f>);

}  // namespace nme::math

#include "details/vec.inl"

// EOF