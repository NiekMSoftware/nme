#pragma once

#include <cmath>

namespace nme::math {

// --------------------------------- dot -------------------------------------

template<typename T, usize N>
constexpr T dot(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    T r = T(0);
    for (usize i = 0; i < N; ++i) r += a[i] * b[i];
    return r;
}

#if defined(NME_SIMD_SSE)
// SSE override, chosen by overload resolution.
inline f32 dot(const Vector<f32, 4>& a, const Vector<f32, 4>& b) noexcept {
#if defined (NME_SIMD_SSE4)
    return _mm_cvtss_f32(_mm_dp_ps(a.v, b.v, 0xF1));
#else
    const __m128 m = _mm_mul_ps(a.v, b.v);
    __m128 s = _mm_hadd_ps(m, m);
    s = _mm_hadd_ps(s, s);
    return _mm_cvtss_f32(s);
#endif
}
#endif

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