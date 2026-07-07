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
    return detail::zip(a, b, [](T x, T y) { return x * y; });
}

template<typename T, usize N>
constexpr Vector<T, N> operator/(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x / y; });
}

// ------------------------- vector <op>= vector -----------------------------

template<typename T, usize N>
constexpr Vector<T, N>& operator+=(Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] += b[i];
    return a;
}

template<typename T, usize N>
constexpr Vector<T, N>& operator-=(Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] -= b[i];
    return a;
}

template<typename T, usize N>
constexpr Vector<T, N>& operator*=(Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] *= b[i];
    return a;
}

template<typename T, usize N>
constexpr Vector<T, N>& operator/=(Vector<T, N>& a, const Vector<T, N>& b) noexcept {
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
constexpr Vector<T, N>& operator*=(Vector<T, N>& v, U s) noexcept {
    const T t = static_cast<T>(s);
    for (usize i = 0; i < N; ++i) v[i] *= t;
    return v;
}

template<typename T, usize N, convertible_to<T> U>
constexpr Vector<T, N>& operator/=(Vector<T, N>& v, U s) noexcept {
    NME_ASSERT(static_cast<T>(s) != T(0));
    const T t = static_cast<T>(s);
    for (usize i = 0; i < N; ++i) v[i] /= t;
    return v;
}

// ---------------------------- bitwise: unary -------------------------------
// Integer element types only. `is_bitwise` excludes bool, since bitwise ops on
// a bool-mask vector are meaningless (use negate/any/all for masks instead).

template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N> operator~(const Vector<T, N>& v) noexcept {
    return detail::map(v, [](T x) { return static_cast<T>(~x); });
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
    return a[N-1] <=> b[N-1];   // equal: return the equal-category of value T
}

// ------------------------- bitwise: vector <op> vector ---------------------

template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N> operator&(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x & y; });
}

template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N> operator|(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x | y; });
}

template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N> operator^(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x ^ y; });
}

// Shifts by a matching vector of counts: each lane shifted by its own count.
template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N> operator<<(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x << y; });
}

template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N> operator>>(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x >> y; });
}

// ------------------------- bitwise: vector <op> scalar ---------------------
// Broadcast the scalar to every lane. Shifts by a scalar are the common case
// (v << 2 shifts every component by 2).

template<typename T, usize N, convertible_to<T> U>
    requires is_bitwise<T>
constexpr Vector<T, N> operator&(const Vector<T, N>& v, U s) noexcept {
    const T t = static_cast<T>(s);
    return detail::map(v, [t](T x) { return static_cast<T>(x & t); });
}

template<typename T, usize N, convertible_to<T> U>
    requires is_bitwise<T>
constexpr Vector<T, N> operator|(const Vector<T, N>& v, U s) noexcept {
    const T t = static_cast<T>(s);
    return detail::map(v, [t](T x) { return static_cast<T>(x | t); });
}

template<typename T, usize N, convertible_to<T> U>
    requires is_bitwise<T>
constexpr Vector<T, N> operator^(const Vector<T, N>& v, U s) noexcept {
    const T t = static_cast<T>(s);
    return detail::map(v, [t](T x) { return static_cast<T>(x ^ t); });
}

template<typename T, usize N, convertible_to<T> U>
    requires is_bitwise<T>
constexpr Vector<T, N> operator<<(const Vector<T, N>& v, U s) noexcept {
    const T t = static_cast<T>(s);
    return detail::map(v, [t](T x) { return static_cast<T>(x << t); });
}

template<typename T, usize N, convertible_to<T> U>
    requires is_bitwise<T>
constexpr Vector<T, N> operator>>(const Vector<T, N>& v, U s) noexcept {
    const T t = static_cast<T>(s);
    return detail::map(v, [t](T x) { return static_cast<T>(x >> t); });
}

// & | ^ with the scalar on the left are commutative; single definitions.
template<typename T, usize N, convertible_to<T> U>
    requires is_bitwise<T>
constexpr Vector<T, N> operator&(U s, const Vector<T, N>& v) noexcept { return v & s; }

template<typename T, usize N, convertible_to<T> U>
    requires is_bitwise<T>
constexpr Vector<T, N> operator|(U s, const Vector<T, N>& v) noexcept { return v | s; }

template<typename T, usize N, convertible_to<T> U>
    requires is_bitwise<T>
constexpr Vector<T, N> operator^(U s, const Vector<T, N>& v) noexcept { return v ^ s; }
// NOTE: no scalar-on-left shift (s << v) - shifting a scalar by a vector of
// counts has no sensible single-result meaning.

// ------------------------- bitwise: vector <op>= vector --------------------

template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N>& operator&=(Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] &= b[i];
    return a;
}

template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N>& operator|=(Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] |= b[i];
    return a;
}

template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N>& operator^=(Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] ^= b[i];
    return a;
}

template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N>& operator<<=(Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] <<= b[i];
    return a;
}

template<typename T, usize N>
    requires is_bitwise<T>
constexpr Vector<T, N>& operator>>=(Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    for (usize i = 0; i < N; ++i) a[i] >>= b[i];
    return a;
}

// ------------------------- bitwise: vector <op>= scalar --------------------

template<typename T, usize N, convertible_to<T> U>
requires is_bitwise<T>
constexpr Vector<T, N>& operator&=(Vector<T, N>& a, U s) {
    for (usize i = 0; i < N; ++i) a[i] &= s;
    return a;
}

template<typename T, usize N, convertible_to<T> U>
requires is_bitwise<T>
constexpr Vector<T, N>& operator|=(Vector<T, N>& a, U s) {
    for (usize i = 0; i < N; ++i) a[i] |= s;
    return a;
}

template<typename T, usize N, convertible_to<T> U>
requires is_bitwise<T>
constexpr Vector<T, N>& operator^=(Vector<T, N>& a, U s) {
    for (usize i = 0; i < N; ++i) a[i] ^= s;
    return a;
}

template<typename T, usize N, convertible_to<T> U>
requires is_bitwise<T>
constexpr Vector<T, N>& operator<<=(Vector<T, N>& a, U s) {
    for (usize i = 0; i < N; ++i) a[i] <<= s;
    return a;
}

template<typename T, usize N, convertible_to<T> U>
requires is_bitwise<T>
constexpr Vector<T, N>& operator>>=(Vector<T, N>& a, U s) {
    for (usize i = 0; i < N; ++i) a[i] >>= s;
    return a;
}

}  // namespace nme::math