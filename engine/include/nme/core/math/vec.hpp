#pragma once

#include "details/vec_base.hpp"
#include "details/vec_traits.hpp"

namespace nme::math {

template<typename T, usize N>
struct Vector : VectorBase<T, N> {

    using type       = Vector<T, N>;
    using value_type = T;
    using size_type  = usize;

    /** @brief Number of components. */
    static constexpr usize comp = N;

    /** @brief Vector base (storage) class. */
    using base = VectorBase<T, N>;

    // ---- introspection / raw access ----

    static constexpr usize size() noexcept;
    constexpr T* data() noexcept;
    [[nodiscard]] constexpr const T* data() const noexcept;

    // ---- constructors ----
    constexpr Vector() noexcept;

    /** @brief Broadcast one scalar to every component. */
    template<convertible_to<T> X>
    explicit constexpr Vector(X s) noexcept;

    /** @brief Any mix of scalars and smaller vectors, flattened left-to-right.
     *         Trailing slots zero-filled; overflow dropped. */
    template<typename... Args>
        requires(sizeof...(Args) >= 2) && vector_component_pack<T, Args...>
    constexpr Vector(const Args&... args) noexcept;

    /** @brief Assign from a vector possibly different size (min copy, zero tail). */
    template<convertible_to<T> U, usize M>
    constexpr Vector<T, N>& operator=(const Vector<U, M>& rhs) noexcept;

    // ---- element access ----

    constexpr T& operator[](usize i) noexcept;
    constexpr const T& operator[](usize i) const noexcept;
};

// --------------------------------------------------------------------------
// Aliases
// --------------------------------------------------------------------------

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

using vec2     = Vector<f32, 2>;
using vec3     = Vector<f32, 3>;
using vec4     = Vector<f32, 4>;

using float2   = Vector<f32, 2>;
using float3   = Vector<f32, 3>;
using float4   = Vector<f32, 4>;

using double2  = Vector<f64, 2>;
using double3  = Vector<f64, 3>;
using double4  = Vector<f64, 4>;

using int2     = Vector<i32, 2>;
using int3     = Vector<i32, 3>;
using int4     = Vector<i32, 4>;

using uint2    = Vector<u32, 2>;
using uint3    = Vector<u32, 3>;
using uint4    = Vector<u32, 4>;

using usize2   = Vector<usize, 2>;
using usize3   = Vector<usize, 3>;
using usize4   = Vector<usize, 4>;

}  // namespace nme::math

// --------------------------------------------------------------------------
// Implementation
// --------------------------------------------------------------------------

// TODO: fill in inl headers in correct order
#include "details/vec_ctor.inl"
#include "details/vec_ops.inl"
#include "details/vec_geometric.inl"
#include "details/vec_common.inl"
#include "details/vec_trig.inl"
#include "details/vec_compare.inl"
#include "details/vec_constants.inl"

// EOF