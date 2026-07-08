#pragma once

#include <algorithm>
#include <cmath>

namespace nme::math {

template<typename T, usize N>
constexpr Vector<T, N> abs(const Vector<T, N> &v) noexcept {
    return detail::map(v, [](T x) { return x < T(0) ? -x : x; });
}

template<typename T, usize N>
constexpr Vector<T, N> min(const Vector<T, N> &a, const Vector<T, N> &b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x < y ? x : y; });
}

template<typename T, usize N>
constexpr Vector<T, N> max(const Vector<T, N> &a, const Vector<T, N> &b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x > y ? x : y; });
}

template<typename T, usize N>
constexpr Vector<T, N> clamp(const Vector<T, N>& v,
                             const Vector<T, N>& lo,
                             const Vector<T, N>& hi) noexcept {
    return min(max(v, lo), hi);
}

template<typename T, usize N, convertible_to<T> U>
constexpr Vector<T, N> clamp(const Vector<T, N>& v, U lo, U hi) noexcept {
    return clamp(v, Vector<T, N>(static_cast<T>(lo)), Vector<T, N>(static_cast<T>(hi)));
}

// Linear interpolation. t in [0,1] but not clamped, pass values outside for
// extrapolation if you want to.
template<typename T, usize N, convertible_to<T> U>
constexpr Vector<T, N> lerp(const Vector<T, N>& a, const Vector<T, N>& b, U t) noexcept {
    const T tt = static_cast<T>(t);
    return detail::zip(a, b, [tt](T x, T y) { return x + (y - x) * tt; });
}

template<typename T, usize N>
constexpr Vector<T, N> floor(const Vector<T, N> &v) noexcept {
    return detail::map(v, [](T x){ return std::floor(x); });
}

template<typename T, usize N>
constexpr Vector<T, N> ceil(const Vector<T, N> &v) noexcept {
    return detail::map(v, [](T x){ return std::ceil(x); });
}

template<typename T, usize N>
constexpr Vector<T, N> round(const Vector<T, N> &v) noexcept {
    return detail::map(v, [](T x){ return std::round(x); });
}

template<typename T, usize N>
    requires is_signed<T>
constexpr Vector<T, N> sign(const Vector<T, N> &v) noexcept {
    return detail::map(v, [](T x){ return T((x > T(0)) - (x < T(0))); });
}

}  // namespace nme::math