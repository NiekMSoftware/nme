// vec.inl - operator and free-function definitions for nme::math vectors.

#pragma once

#include "../concepts.h"

namespace nme::math {

// ===========================================================================
//                          Arithmetic operators
// ===========================================================================

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
constexpr Vector<T, N> operator*(Vector<T, N> v, type_identity_t<T> s) {
    for (u32 i = 0; i < N; ++i) v.data[i] *= s;
    return v;
}
template<typename T, u32 N>
constexpr Vector<T, N> operator*(type_identity_t<T> s, Vector<T, N> v) {
    return v * s;
}
template<typename T, u32 N>
constexpr Vector<T, N> operator/(Vector<T, N> v, type_identity_t<T> s) {
    for (u32 i = 0; i < N; ++i) v.data[i] /= s;
    return v;
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
constexpr Vector<T, N>& operator*=(Vector<T, N>& v, type_identity_t<T> s) {
    for (u32 i = 0; i < N; ++i) v.data[i] *= s;
    return v;
}
template<typename T, u32 N>
constexpr Vector<T, N>& operator/=(Vector<T, N>& v, type_identity_t<T> s) {
    for (u32 i = 0; i < N; ++i) v.data[i] /= s;
    return v;
}

// ===========================================================================
//                            Bitwise operators
// ===========================================================================

#define NME_DEFINE_BITWISE(op)                                                  \
    template<typename T, u32 N>                                                 \
    constexpr std::enable_if_t<is_bitwise<T>, Vector<T, N>>             \
    operator op(Vector<T, N> a, const Vector<T, N>& b) {                        \
        for (u32 i = 0; i < N; ++i) a.data[i] op##= b.data[i];                  \
        return a;                                                               \
    }                                                                           \
    template<typename T, u32 N>                                                 \
    constexpr std::enable_if_t<is_bitwise<T>, Vector<T, N>>             \
    operator op(Vector<T, N> a, type_identity_t<T> s) {                 \
        for (u32 i = 0; i < N; ++i) a.data[i] op##= s;                          \
        return a;                                                               \
    }                                                                           \
    template<typename T, u32 N>                                                 \
    constexpr std::enable_if_t<is_bitwise<T>, Vector<T, N>&>            \
    operator op##=(Vector<T, N>& a, const Vector<T, N>& b) {                    \
        for (u32 i = 0; i < N; ++i) a.data[i] op##= b.data[i];                  \
        return a;                                                               \
    }                                                                           \
    template<typename T, u32 N>                                                 \
    constexpr std::enable_if_t<is_bitwise<T>, Vector<T, N>&>            \
    operator op##=(Vector<T, N>& a, type_identity_t<T> s) {             \
        for (u32 i = 0; i < N; ++i) a.data[i] op##= s;                          \
        return a;                                                               \
    }

NME_DEFINE_BITWISE(&)
NME_DEFINE_BITWISE(|)
NME_DEFINE_BITWISE(^)
NME_DEFINE_BITWISE(<<)
NME_DEFINE_BITWISE(>>)
#undef NME_DEFINE_BITWISE

template<typename T, u32 N>
constexpr std::enable_if_t<is_bitwise<T>, Vector<T, N>>
operator~(Vector<T, N> v) {
    for (u32 i = 0; i < N; ++i) v.data[i] = static_cast<T>(~v.data[i]);
    return v;
}

// ===========================================================================
//                              Comparison
// ===========================================================================

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

#define NME_DEFINE_MASK(name, op)                                               \
    template<typename T, u32 N>                                                 \
    [[nodiscard]] constexpr Vector<bool, N> name(const Vector<T, N>& a,         \
                                                 const Vector<T, N>& b) {       \
        Vector<bool, N> m{};                                                    \
        for (u32 i = 0; i < N; ++i) m.data[i] = a.data[i] op b.data[i];         \
        return m;                                                               \
    }
NME_DEFINE_MASK(less_than, <)     NME_DEFINE_MASK(greater_than, >)
NME_DEFINE_MASK(less_equal, <=)   NME_DEFINE_MASK(greater_equal, >=)
NME_DEFINE_MASK(equal, ==)        NME_DEFINE_MASK(not_equal, !=)
#undef NME_DEFINE_MASK

template<u32 N>
[[nodiscard]] constexpr bool any(const Vector<bool, N>& m) {
    for (u32 i = 0; i < N; ++i) if (m.data[i]) return true;
    return false;
}
template<u32 N>
[[nodiscard]] constexpr bool all(const Vector<bool, N>& m) {
    for (u32 i = 0; i < N; ++i) if (!m.data[i]) return false;
    return true;
}
template<typename T, u32 N>
[[nodiscard]] constexpr Vector<T, N> select(const Vector<bool, N>& m,
                                            const Vector<T, N>& if_true,
                                            const Vector<T, N>& if_false) {
    Vector<T, N> r{};
    for (u32 i = 0; i < N; ++i) r.data[i] = m.data[i] ? if_true.data[i] : if_false.data[i];
    return r;
}

// ===========================================================================
//                              Geometric
// ===========================================================================

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
[[nodiscard]] constexpr T length_squared(const Vector<T, N>& v) { return dot(v, v); }

template<typename T, u32 N>
[[nodiscard]] constexpr T distance_squared(const Vector<T, N>& a, const Vector<T, N>& b) {
    return length_squared(b - a);
}

template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<is_floating<T>, T>
length(const Vector<T, N>& v) { return std::sqrt(length_squared(v)); }

template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<is_floating<T>, T>
distance(const Vector<T, N>& a, const Vector<T, N>& b) { return length(b - a); }

// Assumes v is non-zero.
template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<is_floating<T>, Vector<T, N>>
normalize(const Vector<T, N>& v) { return v * (T{1} / length(v)); }

template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<is_floating<T>, Vector<T, N>>
normalize_or_zero(const Vector<T, N>& v, type_identity_t<T> eps = T(1e-8)) {
    T len2 = length_squared(v);
    return len2 > eps * eps ? v * (T{1} / std::sqrt(len2)) : Vector<T, N>{};
}

// n is assumed unit length.
template<typename T, u32 N>
[[nodiscard]] constexpr std::enable_if_t<is_floating<T>, Vector<T, N>>
reflect(const Vector<T, N>& i, const Vector<T, N>& n) {
    return i - n * (T{2} * dot(i, n));
}

template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<is_floating<T>, Vector<T, N>>
refract(const Vector<T, N>& i, const Vector<T, N>& n, type_identity_t<T> eta) {
    T ni = dot(n, i);
    T k = T{1} - eta * eta * (T{1} - ni * ni);
    if (k < T{0}) return Vector<T, N>{};
    return i * eta - n * (eta * ni + std::sqrt(k));
}

// a projected onto `onto` (onto assumed non-zero); reject is the perpendicular part.
template<typename T, u32 N>
[[nodiscard]] constexpr std::enable_if_t<is_floating<T>, Vector<T, N>>
project(const Vector<T, N>& a, const Vector<T, N>& onto) {
    return onto * (dot(a, onto) / dot(onto, onto));
}
template<typename T, u32 N>
[[nodiscard]] constexpr std::enable_if_t<is_floating<T>, Vector<T, N>>
reject(const Vector<T, N>& a, const Vector<T, N>& from) {
    return a - project(a, from);
}

template<typename T, u32 N>
[[nodiscard]] constexpr std::enable_if_t<is_floating<T>, Vector<T, N>>
faceforward(const Vector<T, N>& n, const Vector<T, N>& i, const Vector<T, N>& nref) {
    return dot(nref, i) < T{0} ? n : -n;
}

template<typename T, u32 N>
[[nodiscard]] constexpr std::enable_if_t<is_floating<T>, bool>
is_near(const Vector<T, N>& a, const Vector<T, N>& b, type_identity_t<T> eps = T(1e-5)) {
    for (u32 i = 0; i < N; ++i) {
        T d = a.data[i] - b.data[i];
        if ((d < T{0} ? -d : d) > eps) return false;
    }
    return true;
}

// ===========================================================================
//                           Component-wise math
// ===========================================================================

template<typename T, u32 N>
[[nodiscard]] constexpr Vector<T, N> abs(Vector<T, N> v) {
    for (u32 i = 0; i < N; ++i) v.data[i] = v.data[i] < T{0} ? -v.data[i] : v.data[i];
    return v;
}
template<typename T, u32 N>
[[nodiscard]] constexpr Vector<T, N> sign(Vector<T, N> v) {
    for (u32 i = 0; i < N; ++i) v.data[i] = T(T{0} < v.data[i]) - T(v.data[i] < T{0});
    return v;
}
template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<is_floating<T>, Vector<T, N>> floor(Vector<T, N> v) {
    for (u32 i = 0; i < N; ++i) v.data[i] = std::floor(v.data[i]);
    return v;
}
template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<is_floating<T>, Vector<T, N>> ceil(Vector<T, N> v) {
    for (u32 i = 0; i < N; ++i) v.data[i] = std::ceil(v.data[i]);
    return v;
}
template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<is_floating<T>, Vector<T, N>> round(Vector<T, N> v) {
    for (u32 i = 0; i < N; ++i) v.data[i] = std::round(v.data[i]);
    return v;
}
template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<is_floating<T>, Vector<T, N>> fract(Vector<T, N> v) {
    for (u32 i = 0; i < N; ++i) v.data[i] = v.data[i] - std::floor(v.data[i]);
    return v;
}
template<typename T, u32 N>
[[nodiscard]] std::enable_if_t<is_floating<T>, Vector<T, N>> sqrt(Vector<T, N> v) {
    for (u32 i = 0; i < N; ++i) v.data[i] = std::sqrt(v.data[i]);
    return v;
}

template<typename T, u32 N>
[[nodiscard]] constexpr Vector<T, N> min(const Vector<T, N>& a, const Vector<T, N>& b) {
    Vector<T, N> r{};
    for (u32 i = 0; i < N; ++i) r.data[i] = a.data[i] < b.data[i] ? a.data[i] : b.data[i];
    return r;
}
template<typename T, u32 N>
[[nodiscard]] constexpr Vector<T, N> max(const Vector<T, N>& a, const Vector<T, N>& b) {
    Vector<T, N> r{};
    for (u32 i = 0; i < N; ++i) r.data[i] = a.data[i] > b.data[i] ? a.data[i] : b.data[i];
    return r;
}
template<typename T, u32 N>
[[nodiscard]] constexpr Vector<T, N> step(const Vector<T, N>& edge, const Vector<T, N>& v) {
    Vector<T, N> r{};
    for (u32 i = 0; i < N; ++i) r.data[i] = v.data[i] < edge.data[i] ? T{0} : T{1};
    return r;
}

template<typename T, u32 N>
[[nodiscard]] constexpr Vector<T, N> clamp(const Vector<T, N>& v,
                                           const Vector<T, N>& lo, const Vector<T, N>& hi) {
    return min(max(v, lo), hi);
}
template<typename T, u32 N>
[[nodiscard]] constexpr Vector<T, N> clamp(const Vector<T, N>& v,
                                           type_identity_t<T> lo,
                                           type_identity_t<T> hi) {
    Vector<T, N> r{};
    for (u32 i = 0; i < N; ++i)
        r.data[i] = v.data[i] < lo ? lo : (v.data[i] > hi ? hi : v.data[i]);
    return r;
}
template<typename T, u32 N>
[[nodiscard]] constexpr std::enable_if_t<is_floating<T>, Vector<T, N>>
saturate(const Vector<T, N>& v) { return clamp(v, T{0}, T{1}); }

template<typename T, u32 N>
[[nodiscard]] constexpr std::enable_if_t<is_floating<T>, Vector<T, N>>
lerp(const Vector<T, N>& a, const Vector<T, N>& b, type_identity_t<T> t) {
    return a + (b - a) * t;
}
template<typename T, u32 N>
[[nodiscard]] constexpr std::enable_if_t<is_floating<T>, Vector<T, N>>
smoothstep(const Vector<T, N>& e0, const Vector<T, N>& e1, const Vector<T, N>& v) {
    Vector<T, N> r{};
    for (u32 i = 0; i < N; ++i) {
        T t = (v.data[i] - e0.data[i]) / (e1.data[i] - e0.data[i]);
        t = t < T{0} ? T{0} : (t > T{1} ? T{1} : t);
        r.data[i] = t * t * (T{3} - T{2} * t);
    }
    return r;
}

template<typename T, u32 N>
[[nodiscard]] constexpr T min_component(const Vector<T, N>& v) {
    T m = v.data[0];
    for (u32 i = 1; i < N; ++i) if (v.data[i] < m) m = v.data[i];
    return m;
}
template<typename T, u32 N>
[[nodiscard]] constexpr T max_component(const Vector<T, N>& v) {
    T m = v.data[0];
    for (u32 i = 1; i < N; ++i) if (v.data[i] > m) m = v.data[i];
    return m;
}
template<typename T, u32 N>
[[nodiscard]] constexpr T component_sum(const Vector<T, N>& v) {
    T s{};
    for (u32 i = 0; i < N; ++i) s += v.data[i];
    return s;
}

}  // namespace nme::math
