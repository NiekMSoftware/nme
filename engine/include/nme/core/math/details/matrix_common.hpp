#pragma once

#include <algorithm>
#include <cmath>

namespace nme::math {

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> abs(const Matrix<T, N, M>& m) noexcept {
    return detail::map(m, [](T x) { return x < T(0) ? -x : x; });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> min(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x < y ? x : y; });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> max(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x > y ? x : y; });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> clamp(const Matrix<T, N, M>& m,
                                const Matrix<T, N, M>& lo,
                                const Matrix<T, N, M>& hi) noexcept {
    return min(max(m, lo), hi);
}

template<typename T, usize N, usize M, convertible_to<T> U>
constexpr Matrix<T, N, M> clamp(const Matrix<T, N, M>& m, U lo, U hi) noexcept {
    return clamp(m, Matrix<T, N, M>(static_cast<T>(lo)), Matrix<T, N, M>(static_cast<T>(hi)));
}

// Linear interpolation: result = a + (b - a) * t.
// t typically ranges [0, 1]
template<typename T, usize N, usize M, convertible_to<T> U>
constexpr Matrix<T, N, M> lerp(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b, U t) noexcept {
    const T tt = static_cast<T>(t);
    return detail::zip(a, b, [tt](T x, T y) { return x + (y - x) * tt; });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> floor(const Matrix<T, N, M>& m) noexcept {
    return detail::map(m, [](T x) { return std::floor(x); });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> ceil(const Matrix<T, N, M>& m) noexcept {
    return detail::map(m, [](T x) { return std::ceil(x); });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> round(const Matrix<T, N, M>& m) noexcept {
    return detail::map(m, [](T x) { return std::round(x); });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> sign(const Matrix<T, N, M>& m) noexcept {
    return detail::map(m, [](T x) { return T((x > T(0)) - (x < T(0))); });
}

// Euclidian norm: sqrt of sum of squared elements.
// https://mathworld.wolfram.com/FrobeniusNorm.html
template<typename T, usize N, usize M>
constexpr T norm_euclidian_sqrt(Matrix<T, N, M> const& m) noexcept {
    T sum = T(0);
    for (usize i = 0; i < N; ++i)
        sum += m.data()[i] * m.data()[i];
    return sum;
}

template<typename T, usize N, usize M>
constexpr T norm_euclidian(Matrix<T, N, M> const& m) noexcept {
    return std::sqrt(norm_euclidian_sqrt(m));
}

// Sum all elements.
template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> sum(const Matrix<T, N, M>& m) noexcept {
    T r = T(0);
    for (usize i = 0; i < N; ++i)
        r += m.data()[i];
    return r;
}

// Product of all elements.
template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> product(const Matrix<T, N, M>& m) noexcept {
    Matrix<T, N, M> r;
    for (usize i = 0; i < N; ++i)
        r *= m.data()[i];
    return r;
}

// Trace: sum of diagonal elements (only for square matrices).
template<typename T, usize N>
constexpr T trace(const Matrix<T, N, N>& m) noexcept {
    T r = T(0);
    for (usize i = 0; i < N; ++i)
        r += m(i, i);
    return r;
}

}  // namespace nme::math