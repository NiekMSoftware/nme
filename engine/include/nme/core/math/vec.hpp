#pragma once

#include <cmath>
#include <type_traits>

#include "nme/platform/types.h"
#include "details/vec_base.hpp"

namespace nme::math {

namespace detail {

template<typename T> struct type_identity { using type = T; };
template<typename T> using type_identity_t = typename type_identity<T>::type;

template<typename T>
inline constexpr bool is_floating = std::is_floating_point_v<T>;

template<typename T> inline constexpr bool is_bitwise =
    std::is_integral_v<T> && !std::is_same_v<T, bool>;

}  // namespace detail

// --------------------------------------------
//                   Storage
// --------------------------------------------

// Generic fallback
template<typename T, usize N> struct Vector { T data[N]; };

#define NME_VEC2_MEMBERS(T)                                                    \
    union {                                                                    \
        T data[2];                                                             \
        struct { T x, y; };                                                    \
    };                                                                         \
    Vector() = default;                                                        \
    constexpr Vector(T x_, T y_) : data{x_, y_} {}                             \
    constexpr explicit Vector(T s) : data{s, s} {}                             \
    template<typename U, std::enable_if_t<!std::is_same_v<U, T>, int> = 0>     \
    constexpr explicit Vector(const Vector<U, 2>& v)                           \
        : data{T(v.data[0]), T(v.data[1])} {}

#define NME_VEC3_MEMBERS(T)                                                    \
    union {                                                                    \
        T data[3];                                                             \
        struct { T x, y, z; };                                                 \
        struct { T r, g, b; };                                                 \
        Vector<T, 2> xy;                                                       \
    };                                                                         \
        Vector() = default;                                                    \
        constexpr Vector(T x_, T y_, T z_) : data{x_, y_, z_} {}               \
        constexpr explicit Vector(T s) : data{s, s, s} {}                      \
        constexpr Vector(const Vector<T, 2>& v, T z_)                          \
            : data{v.data[0], v.data[1], z_} {}                                \
        template<typename U, std::enable_if_t<!std::is_same_v<U, T>, int> = 0> \
        constexpr explicit Vector(const Vector<U, 3>& v)                       \
            : data{T(v.data[0]), T(v.data[1]), T(v.data[2])} {}

#define NME_VEC4_MEMBERS(T)                                                    \
    union {                                                                    \
        T data[4];                                                             \
        struct { T x, y, z, w; };                                              \
        struct { T r, g, b, a; };                                              \
        Vector<T, 2> xy;                                                       \
        Vector<T, 3> xyz;                                                      \
        Vector<T, 3> rgb;                                                      \
    };                                                                         \
    Vector() = default;                                                        \
    constexpr Vector(T x_, T y_, T z_, T w_) : data{x_, y_, z_, w_} {}         \
    constexpr explicit Vector(T s) : data{s, s, s, s} {}                       \
    constexpr Vector(const Vector<T, 3>& v, T w_)                              \
        : data{v.data[0], v.data[1], v.data[2], w_} {}                         \
    constexpr Vector(const Vector<T, 2>& v, T z_, T w_)                        \
        : data{v.data[0], v.data[1], z_, w_} {}                                \
    constexpr Vector(const Vector<T, 2>& lo, const Vector<T, 2>& hi)           \
        : data{lo.data[0], lo.data[1], hi.data[0], hi.data[1]} {}              \
    template<typename U, std::enable_if_t<!std::is_same_v<U, T>, int> = 0>     \
    constexpr explicit Vector(const Vector<U, 4>& v)                           \
        : data{T(v.data[0]), T(v.data[1]), T(v.data[2]), T(v.data[3])} {}

// NOLINTBEGIN(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
// ---------- Vector2 ----------
template <> struct Vector<f32, 2> { NME_VEC2_MEMBERS(f32) };
template <> struct Vector<f64, 2> { NME_VEC2_MEMBERS(f64); };
template <> struct Vector<i32, 2> { NME_VEC2_MEMBERS(i32); };

// ---------- Vector3 ----------
template <> struct Vector<f32, 3> { NME_VEC3_MEMBERS(f32); };
template <> struct Vector<f64, 3> { NME_VEC3_MEMBERS(f64); };
template <> struct Vector<i32, 3> { NME_VEC3_MEMBERS(i32); };

// ---------- Vector 4 ----------
template <> struct alignas(16) Vector<f32, 4> { NME_VEC4_MEMBERS(f32); };
template <> struct alignas(16) Vector<i32, 4> { NME_VEC4_MEMBERS(i32); };
template <> struct             Vector<f64, 4> { NME_VEC4_MEMBERS(f64); };
// NOLINTEND(cppcoreguidelines-pro-type-member-init,hicpp-member-init)

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
static_assert(sizeof(Vector2f) == 8);
static_assert(sizeof(Vector3f) == 12);
static_assert(sizeof(Vector4f) == 16 && alignof(Vector4f) == 16);
static_assert(sizeof(Vector4i) == 16 && alignof(Vector4i) == 16);
static_assert(std::is_trivially_copyable_v<Vector4f>);
static_assert(std::is_standard_layout_v<Vector4f>);

}  // namespace nme::math

#include "details/vec.inl"

// EOF