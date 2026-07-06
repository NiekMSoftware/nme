#pragma once

#include <cmath>

namespace nme::math {

template<typename T, usize N>
constexpr Vector<T, N> radians(const Vector<T, N>& deg) noexcept {
    constexpr T k = PI<T> / T(180);
    return detail::map(deg, [](T x) { return x * k; });
}

template<typename T, usize N>
constexpr Vector<T, N> degrees(const Vector<T, N>& rad) noexcept {
    constexpr T k = T(180) / PI<T>;
    return detail::map(rad, [](T x) { return x * k; });
}

template<typename T, usize N>
Vector<T, N> sin(const Vector<T, N>& v) noexcept {
    return detail::map(v, [](T x) { return std::sin(x); });
}

template<typename T, usize N>
Vector<T, N> cos(const Vector<T, N>& v) noexcept {
    return detail::map(v, [](T x) { return std::cos(x); });
}

template<typename T, usize N>
Vector<T, N> tan(const Vector<T, N>& v) noexcept {
    return detail::map(v, [](T x) { return std::tan(x); });
}

template<typename T, usize N>
Vector<T, N> asin(const Vector<T, N>& v) noexcept {
    return detail::map(v, [](T x) { return std::asin(x); });
}

template<typename T, usize N>
Vector<T, N> acos(const Vector<T, N>& v) noexcept {
    return detail::map(v, [](T x) { return std::acos(x); });
}

template<typename T, usize N>
Vector<T, N> atan(const Vector<T, N>& v) noexcept {
    return detail::map(v, [](T x) { return std::atan(x); });
}

template<typename T, usize N>
Vector<T, N> atan2(const Vector<T, N>& y, const Vector<T, N>& x) noexcept {
    return detail::zip(y, x,  [](T a, T b) { return std::atan2(a, b); });
}

}  // namespace nme::math