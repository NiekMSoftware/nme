#pragma once

namespace nme::math {

template<typename T, usize N>
constexpr Vector<bool, N> lessThan(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    Vector<bool, N> r;
    for (usize i = 0; i < N; ++i) r[i] = a[i] < b[i];
    return r;
}

template<typename T, usize N>
constexpr Vector<bool, N> lessThanEqual(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    Vector<bool, N> r;
    for (usize i = 0; i < N; ++i) r[i] = a[i] <= b[i];
    return r;
}

template<typename T, usize N>
constexpr Vector<bool, N> greaterThan(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    Vector<bool, N> r;
    for (usize i = 0; i < N; ++i) r[i] = a[i] > b[i];
    return r;
}

template<typename T, usize N>
constexpr Vector<bool, N> greaterThanEqual(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    Vector<bool, N> r;
    for (usize i = 0; i < N; ++i) r[i] = a[i] >= b[i];
    return r;
}

// --------------------------- boolean reductions ----------------------------

template<usize N>
constexpr bool any(const Vector<bool, N>& v) noexcept {
    for (usize i = 0; i < N; ++i) if (v[i]) return true;
    return false;
}

template<usize N>
constexpr bool all(const Vector<bool, N>& v) noexcept {
    for (usize i = 0; i < N; ++i) if (!v[i]) return false;
    return true;
}

template<usize N>
constexpr Vector<bool, N> negate(const Vector<bool, N>& v) noexcept {
    Vector<bool, N> r;
    for (usize i = 0; i < N; ++i) r[i] = !v[i];
    return r;
}

// ---------------------------- tolerant equality ----------------------------
// Per component |a-b| <= epsilon. Returns a bool-vector; wrap in all() for a
// single bool. Scalar-epsilon overload broadcasts.

template<typename T, usize N>
constexpr Vector<bool, N> equal(const Vector<bool, N>& a, const Vector<bool, N>& b,
                                const Vector<T, N>& epsilon) noexcept {
    return lessThanEqual(abs(a - b), epsilon);
}

template<typename T, usize N, convertible_to<T> U>
constexpr Vector<bool, N> equal(const Vector<bool, N>& a, const Vector<bool, N>& b,
                                U epsilon = EPSILON<T>) noexcept {
    return equal(a, b, Vector<T, N>(static_cast<T>(epsilon)));
}

template<typename T, usize N>
constexpr Vector<bool, N> notEqual(const Vector<bool, N>& a, const Vector<bool, N>& b,
                                   const Vector<T, N>& epsilon) noexcept {
    return negate(equal(a, b, epsilon));
}

template<typename T, usize N, convertible_to<T> U>
constexpr Vector<bool, N> notEqual(const Vector<bool, N>& a, const Vector<bool, N>& b,
                                   U epsilon = EPSILON<T>) noexcept {
    return negate(equal(a, b, epsilon));
}

}  // namespace nme::math
