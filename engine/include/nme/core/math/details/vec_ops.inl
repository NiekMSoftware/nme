#pragma once

namespace nme::math {

namespace detail {

template<typename T, usize N, typename F>
constexpr Vector<T, N> map(const Vector<T, N>& a, F f) noexcept {
    Vector<T, N> r;
    for (usize i = 0; i < N; ++i) r[i] = f(a[i]);
    return r;
}

template<typename T, usize N, typename F>
constexpr Vector<T, N> zip(const Vector<T, N>& a, const Vector<T, N>& b, F f) noexcept {
    Vector<T, N> r;
    for (usize i = 0; i < N; ++i) r[i] = f(a[i], b[i]);
    return r;
}

}  // detail

// -------------------------------- unary ------------------------------------

template<typename T, usize N>
constexpr Vector<T, N> operator+(const Vector<T, N>& v) noexcept {
    return v;
}

template<typename T, usize N>
constexpr Vector<T, N> operator-(const Vector<T, N>& v) noexcept {
    return detail::map(v, [](T x) { return -x; });
}

// ------------------------- vector <op> vector ------------------------------

template<typename T, usize N>
constexpr Vector<T, N> operator+(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x + y; });
}

template<typename T, usize N>
constexpr Vector<T, N> operator-(const Vector<T, N>& a, const Vector<T, N>& b) {
    return detail::zip(a, b, [](T x, T y) { return x - y; });
}

template<typename T, usize N>
constexpr Vector<T, N> operator*(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return detail::zip(a, [](T x, T y) { return x * y; });
}

template<typename T, usize N>
constexpr Vector<T, N> operator/(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return detail::zip(a, [](T x, T y) { return x / y; });
}

// ------------------------- vector <op>= vector -----------------------------

template<typename T, usize N>
constexpr Vector<T, N>& operator+=(Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] += b[i];
    return a;
}

template<typename T, usize N>
constexpr Vector<T, N>& operator-=(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] -= b[i];
    return a;
}

template<typename T, usize N>
constexpr Vector<T, N>& operator*=(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] *= b[i];
    return a;
}

template<typename T, usize N>
constexpr Vector<T, N>& operator/=(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] /= b[i];
    return a;
}

// ------------------------- vector <op> scalar ------------------------------

template<typename T, usize N, convertible_to<T> U>
constexpr Vector<T, N> operator*(const Vector<T, N>& v, U s) noexcept {
    const T t = static_cast<T>(s);
    return detail::map(v, [t](T x) { return x * t; });
}

template<typename T, usize N, convertible_to<T> U>
constexpr Vector<T, N> operator*(U s, const Vector<T, N>& v) noexcept {
    return v * s;   // commutative; single definition
}

template<typename T, usize N, convertible_to<T> U>
constexpr Vector<T, N> operator/(const Vector<T, N>& v, U s) noexcept {
    NME_ASSERT(static_cast<T>(s) != T(0));
    const T t = static_cast<T>(s);
    return detail::map(v, [t](T x) { return x / t; });
}

template<typename T, usize N, convertible_to<T> U>
constexpr Vector<T, N> operator*=(Vector<T, N>& v, U s) noexcept {
    const T t = static_cast<T>(s);
    for (usize i = 0; i < N; ++i) v[i] *= t;
    return v;
}

template<typename T, usize N, convertible_to<T> U>
constexpr Vector<T, N> operator/=(Vector<T, N>& v, U s) noexcept {
    NME_ASSERT(static_cast<T>(s) != T(0));
    const T t = static_cast<T>(s);
    for (usize i = 0; i < N; ++i) v[i] /= t;
    return v;
}

// ------------------------------ comparison ---------------------------------

template<typename T, usize N>
constexpr bool operator==(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) if (!(a[i] == b[i])) return false;
    return true;
}

// <=> gives lexicographic ordering, mostly for container/sort use,
// not geometric meaning.
template<typename T, usize N>
constexpr auto operator<=>(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i)
        if (auto c = a[i] <=> b[i]; c != 0) return c;
    return a[0] <=> b[0];   // equal: return the equal-category of value T
}

}  // namespace nme::math