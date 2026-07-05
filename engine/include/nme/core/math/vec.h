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

}  // namespace detail

// --------------------------------------------
//                   Storage
// --------------------------------------------

// Generic fallback
template<typename T, u32 N> struct Vector { T data[N]; };

#define NME_VEC2_MEMBERS(T)         \
    union {                         \
        T data[2];                  \
        struct { T x, y; };         \
    };

#define NME_VEC3_MEMBERS(T)         \
    union {                         \
        T data[3];                  \
        struct { T x, y, z; };      \
        struct { T r, g, b; };      \
        Vector<T, 2> xy;            \
    }

#define NME_VEC4_MEMBERS(T)         \
    union {                         \
        T data[4];                  \
        struct { T x, y, z, w; };   \
        struct { T r, g, b, a; };   \
        Vector<T, 2> xy;            \
        Vector<T, 3> xyz;           \
        Vector<T, 3> rgb;           \
    }

// ---------- Vector2 ----------
template <> struct Vector<f32, 2> { NME_VEC2_MEMBERS(f32); };
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

// --------------------------------------------
//            Arithmatic Operators
// --------------------------------------------

template<typename T, u32 N>
constexpr Vector<T, N> operator-(Vector<T, N> v) {
    for (u32 i = 0; i < N; ++i) v.data[i] = -v.data[i];
    return v;
}

template<typename T, u32 N>
constexpr Vector<T, N> operator+(Vector<T, N> a, const Vector<T, N>& b) {
    for (u32 i = 0; i < N; ++i) a.data[i] += b.data[i];
    return a;
}

template<typename T, u32 N>
constexpr Vector<T, N> operator-(Vector<T, N> a, const Vector<T, N>& b) {
    for (u32 i = 0; i < N; ++i) a.data[i] -= b.data[i];
    return a;
}

template<typename T, u32 N>
constexpr Vector<T, N> operator*(Vector<T, N> a, const Vector<T, N>& b) {
    for (u32 i = 0; i < N; ++i) a.data[i] *= b.data[i];
    return a;
}

template<typename T, u32 N>
constexpr Vector<T, N> operator/(Vector<T, N> a, const Vector<T, N>& b) {
    for (u32 i = 0; i < N; ++i) a.data[i] /= b.data[i];
    return a;
}

template<typename T, u32 N>
constexpr Vector<T, N> operator*(Vector<T, N> a, detail::type_identity_t<T> s) {
    for (u32 i = 0; i < N; ++i) a.data[i] *= s;
    return a;
}

template<typename T, u32 N>
constexpr Vector<T, N> operator*(detail::type_identity_t<T> s, Vector<T, N> a) {
    for (u32 i = 0; i < N; ++i) a.data[i] *= s;
    return a;
}

template<typename T, u32 N>
constexpr Vector<T, N> operator/(Vector<T, N> a, detail::type_identity_t<T> s) {
    for (u32 i = 0; i < N; ++i) a.data[i] /= s;
    return a;
}

template<typename T, u32 N>
constexpr Vector<T, N>& operator+=(Vector<T, N>& a, const Vector<T, N>& b) {
    for (u32 i = 0; i < N; ++i) a.data[i] += b.data[i];
    return a;
}

template<typename T, u32 N>
constexpr Vector<T, N>& operator-=(Vector<T, N>& a, const Vector<T, N>& b) {
    for (u32 i = 0; i < N; ++i) a.data[i] -= b.data[i];
    return a;
}

template<typename T, u32 N>
constexpr Vector<T, N>& operator*=(Vector<T, N>& a, detail::type_identity_t<T> s) {
    for (u32 i = 0; i < N; ++i) a.data[i] *= s;
    return a;
}

template<typename T, u32 N>
constexpr Vector<T, N>& operator/=(Vector<T, N>& a, detail::type_identity_t<T> s) {
    for (u32 i = 0; i < N; ++i) a.data[i] /= s;
    return a;
}

template<typename T, u32 N>
constexpr bool operator==(const Vector<T, N>& a, const Vector<T, N>& b) {
    for (u32 i = 0; i < N; ++i)
        if (a.data[i] != b.data[i]) return false;
    return true;
}

template<typename T, u32 N>
constexpr bool operator!=(const Vector<T, N>& a, const Vector<T, N>& b) {
    return !(a == b);
}

// --------------------------------------------
//            Geometric Operations
// --------------------------------------------

template<typename T, u32 N>
[[nodiscard]] constexpr T dot(const Vector<T, N>& a, const Vector<T, N>& b) {
    T acc{};
    for (u32 i = 0; i < N; ++i) acc += a.data[i] * b.data[i];
    return acc;
}

template<typename T>
[[nodiscard]] constexpr Vector<T, 3> cross(const Vector<T, 3>& a, const Vector<T, 3>& b) {
    Vector<T, 3> r{};
    r.data[0] = a.data[1] * b.data[2] - a.data[2] * b.data[1];
    r.data[1] = a.data[2] * b.data[0] - a.data[0] * b.data[2];
    r.data[2] = a.data[0] * b.data[1] - a.data[1] * b.data[0];
    return r;
}

template<typename T, u32 N>
[[nodiscard]] constexpr T length_squared(const Vector<T, N>& v) {
    return dot(v, v);
}

template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<detail::is_floating<T>, T>
length(const Vector<T, N>& v) {
    return std::sqrt(length_squared(v));
}

template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<detail::is_floating<T>, T>
distance(const Vector<T, N>& a, const Vector<T, N>& b) {
    return length(b - a);
}

// Assumes v is non-zero
template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<detail::is_floating<T>, Vector<T, N>>
normalize(const Vector<T, N>& v) {
    return v * (T{1} / length(v));
}

// n is assumed unit length
template<typename T, u32 N>
[[nodiscard]] constexpr std::enable_if_t<detail::is_floating<T>, Vector<T, N>>
reflect(const Vector<T, N>& i, const Vector<T, N>& n) {
    return i - n * (T{2} * dot(i, n));
}

}  // namespace nme::math

// EOF