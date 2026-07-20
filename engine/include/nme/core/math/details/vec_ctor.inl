#pragma once

#include "nme/platform/debug/assert.h"

namespace nme::math {

// ----------------------------- element access ------------------------------

template<typename T, usize N>
constexpr usize Vector<T, N>::size() noexcept { return N; }

template<typename T, usize N>
constexpr T* Vector<T, N>::data() noexcept { return &base::vec[0]; }

template<typename T, usize N>
constexpr const T* Vector<T, N>::data() const noexcept { return &base::vec[0]; }

template <typename T, usize N>
constexpr T& Vector<T, N>::operator[](usize i) noexcept {
    NME_ASSERT(i < N);
    return base::vec[i];
}

template<typename T, usize N>
constexpr const T& Vector<T, N>::operator[](usize i) const noexcept {
    NME_ASSERT(i < N);
    return base::vec[i];
}

// ------------------------------ constructors -------------------------------

template <typename T, usize N>
constexpr Vector<T, N>::Vector() noexcept = default;

template <typename T, usize N>
template <convertible_to<T> X>
constexpr Vector<T, N>::Vector(X s) noexcept {
    for (usize i = 0; i < N; ++i)
        base::vec[i] = static_cast<T>(s);
}

// --- component-pack helpers (write one argument, advance the cursor) ---
namespace detail {

template<typename T, usize N, convertible_to<T> S>
constexpr void vec_emplace(T (&dst)[N], usize& i, const S& s) noexcept {
    if (i < N) dst[i++] = static_cast<T>(s);
}

template<typename T, usize N, typename U, usize M>
constexpr void vec_emplace(T (&dst)[N], usize& i, const Vector<U, M>& v) noexcept {
    for (usize k = 0; k < M && i < N; ++k)
        dst[i++] = static_cast<T>(v[k]);
}

}  // namespace detail

// Any mix of scalars and smaller vectors, flattened left-to-right.
template <typename T, usize N>
template <typename... Args>
    requires(sizeof...(Args) >= 2) && vector_component_pack<T, Args...>
constexpr Vector<T, N>::Vector(const Args&... args) noexcept {
    usize i = 0;
    (detail::vec_emplace(base::vec, i, args), ...);
    for (; i < N; ++i)
        base::vec[i] = T(0);
}

// --------------------------------- assign ----------------------------------
// Copies min(N,M) components; if rhs is smaller, the tail is zero-filled so
// there is never stale data left behind.

template <typename T, usize N>
template<convertible_to<T> U, usize M>
constexpr Vector<T, N>& Vector<T, N>::operator=(const Vector<U, M>& rhs) noexcept {
    constexpr usize c = (N < M) ? N : M;
    usize i = 0;
    for (; i < c; ++i)
        base::vec[i] = static_cast<T>(rhs[i]);
    for (; i < N; ++i)
        base::vec[i] = T(0);
    return *this;
}

}  // nme::math