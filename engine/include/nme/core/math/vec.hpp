#pragma once

#include <cmath>
#include <type_traits>

#include "nme/platform/types.h"

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
template<typename T, u32 N> struct Vector { T data[N]; };

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

// --- f32 vectors ---
typedef Vector<f32, 2> Vector2f;    // Regular Vector2 using default floating-point precision
typedef Vector<f32, 3> Vector3f;    // Regular Vector3 using default floating-point precision
typedef Vector<f32, 4> Vector4f;    // Regular Vector4 using default floating-point precision

// --- f64 vectors ---
typedef Vector<f64, 2> Vector2d;    // Vector2 using a double as floating-point precision
typedef Vector<f64, 3> Vector3d;    // Vector3 using a double as floating-point precision
typedef Vector<f64, 4> Vector4d;    // Vector4 using a double as floating-point precision

// --- i32 vectors ---
typedef Vector<i32, 2> Vector2i;    // Vector2 using a signed integer as precision
typedef Vector<i32, 3> Vector3i;    // Vector3 using a signed integer as precision
typedef Vector<i32, 4> Vector4i;    // Vector4 using a signed integer as precision

// Lock the layout so it can't drift silently across compilers
static_assert(sizeof(Vector2f) == 8);
static_assert(sizeof(Vector3f) == 12);
static_assert(sizeof(Vector4f) == 16 && alignof(Vector4f) == 16);
static_assert(sizeof(Vector4i) == 16 && alignof(Vector4i) == 16);
static_assert(std::is_trivially_copyable_v<Vector4f>);
static_assert(std::is_standard_layout_v<Vector4f>);

}  // namespace nme::math

#include "vec.inl"

// EOF