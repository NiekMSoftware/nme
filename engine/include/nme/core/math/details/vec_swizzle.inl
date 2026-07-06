#pragma once

namespace nme::math {

// Single index -> scalar.
template<usize X, typename T, usize N>
constexpr T swizzle(const Vector<T, N>& v) noexcept {
    static_assert(X < N, "swizzle index out of range");
    return v[X];
}

// Two-or-more indices -> Vector of matching rank.
template<usize X, usize Y, typename T, usize N>
constexpr Vector<T, 2> swizzle(const Vector<T, N>& v) noexcept {
    static_assert(X < N && Y < N, "swizzle index out of range");
    return { v[X], v[Y] };
}

template<usize X, usize Y, usize Z, typename T, usize N>
constexpr Vector<T, 3> swizzle(const Vector<T, N>& v) noexcept {
    static_assert(X < N && Y < N && Z < N, "swizzle index out of range");
    return { v[X], v[Y], v[Z] };
}

template<usize X, usize Y, usize Z, usize W, typename T, usize N>
constexpr Vector<T, 4> swizzle(const Vector<T, N>& v) noexcept {
    static_assert(X < N && Y < N && Z < N && W < N, "swizzle index out of range");
    return { v[X], v[Y], v[Z], v[W] };
}

}  // namespace nme::math
