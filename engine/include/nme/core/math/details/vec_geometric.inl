#pragma once

namespace nme::math {

// --------------------------------- dot -------------------------------------

template<typename T, usize N>
constexpr T dot(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    T r = T(0);
    for (usize i = 0; i < N; ++i) r += a[i] * b[i];
    return r;
}

// TODO: SIMD override

// -------------------------------- cross ------------------------------------
// Only defined for 3-component vectors. cross() on a vec2/vec4 is a hard error.

template<typename T, usize N>
    requires(N == 3)
constexpr Vector<T, N> cross(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    };
}

// ------------------------- length / distance -------------------------------

template<typename T, usize N>
constexpr T length_sqr(const Vector<T, N>& v) noexcept {
    return dot(v, v);
}

template<typename T, usize N>
constexpr T length(const Vector<T, N>& v) noexcept {
    return std::sqrt(length_sqr(v));
}

template<typename T, usize N>
constexpr T distance(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return length(b - a);
}

// ------------------------------ normalize ----------------------------------

template<typename T, usize N>
constexpr Vector<T, N> normalize(const Vector<T, N>& v) noexcept {
    const T l = length(v);
    return (l > T(0)) ? v / l : v;
}

template<typename T, usize N>
constexpr bool normalized(const Vector<T, N>& v, T epsilon = EPSILON<T>) noexcept {
    return std::abs(T(1) - length_sqr(v)) < epsilon;
}

// -------------------------- reflect / project ------------------------------
// n is assumed normalized. reflect: incident i off surface with normal n.

template<typename T, usize N>
constexpr Vector<T, N> reflect(const Vector<T, N>& i, const Vector<T, N>& n) noexcept {
    return i - n * (T(2) * dot(i, n));
}

template<typename T, usize N>
constexpr Vector<T, N> project(const Vector<T, N>& a, const Vector<T, N>& onto) noexcept {
    return onto * (dot(a, onto) / dot(onto, onto));
}

}  // namespace nme::math