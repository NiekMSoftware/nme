#pragma once

#include "nme/core/assert/assert.h"

namespace nme::math {

// ----------------------------- element access -------------------------------

template<typename T, usize N, usize M>
constexpr usize Matrix<T, N, M>::rows() noexcept { return N; }

template<typename T, usize N, usize M>
constexpr usize Matrix<T, N, M>::cols() noexcept { return M; }

template <typename T, usize N, usize M>
constexpr usize Matrix<T, N, M>::size() noexcept { return N * M; }

template <typename T, usize N, usize M>
constexpr T* Matrix<T, N, M>::data() noexcept { return &base::m[0]; }

// Row access: m[i] returns a Vector<T, M> (reference or const-ref).
template <typename T, usize N, usize M>
constexpr Vector<T, M>& Matrix<T, N, M>::operator[](usize i) noexcept {
    NME_ASSERT(i < N);
    return base::row[i];
}
template <typename T, usize N, usize M>
constexpr const Vector<T, M>& Matrix<T, N, M>::operator[](usize i) const noexcept {
    NME_ASSERT(i < N);
    return base::row[i];
}

template <typename T, usize N, usize M>
constexpr T& Matrix<T, N, M>::operator()(const usize i, const usize j) noexcept {
    NME_ASSERT(i < N && j < M);
    return base::m[i * M + j];
}

template <typename T, usize N, usize M>
constexpr const T& Matrix<T, N, M>::operator()(const usize i, const usize j) const noexcept {
    NME_ASSERT(i < N && j < M);
    return base::m[i * M + j];
}

// ------------------------------ constructors --------------------------------

template <typename T, usize N, usize M>
constexpr Matrix<T, N, M>::Matrix() noexcept = default;

template <typename T, usize N, usize M>
template <convertible_to<T> X>
constexpr Matrix<T, N, M>::Matrix(X s) noexcept {
    for (usize i = 0; i < N; ++i)
        base::m[i] = static_cast<T>(s);
}

// Component-pack constructor: mix of scalars and smaller matrices/vectors.
// Trailing slots zero-filled; overflow dropped.
namespace detail {

template<typename T, usize Size, convertible_to<T> S>
constexpr void mat_emplace(T (&dst)[Size], usize& i, const S& s) noexcept {
    if (i < Size) dst[i++] = static_cast<T>(s);
}

template<typename T, usize Size, typename U, usize K>
constexpr void mat_emplace(T (&dst)[Size], usize& i, const Vector<U, K>& v) noexcept {
    for (usize k = 0; k < K && i < Size; ++k)
        dst[i++] = static_cast<T>(v[k]);
}

template<typename T, usize Size, typename U, usize K, usize L>
constexpr void mat_emplace(T (&dst)[Size], usize& i, const Matrix<U, K, L>& m) noexcept {
    for (usize k = 0; k < K * L && i < Size; ++k)
        dst[i++] = static_cast<T>(m.data()[k]);
}

}  // namespace detail

template<typename T, usize N, usize M>
template<typename... Args>
    requires(sizeof...(Args) >= 2) && matrix_component_pack<T, Args...>
constexpr Matrix<T, N, M>::Matrix(const Args&... args) noexcept {
    usize i = 0;
    (detail::mat_emplace(base::m, i, args), ...);
    for (; i < N * M; ++i)
        base::m[i] = T(0);
}

// Initialize from a larger or smaller matrix (min copy, zero-fill tail if needed).
template <typename T, usize N, usize M>
template <convertible_to<T> U, usize K, usize L>
constexpr Matrix<T, N, M>::Matrix(const Matrix<U, K, L>& other) noexcept {
    const usize rows_to_copy = (N < K) ? N : K;
    const usize cols_to_copy = (M < L) ? M : L;

    // Copy the overlapping region
    for (usize i = 0; i < rows_to_copy; ++i)
        for (usize j = 0; j < cols_to_copy; ++j)
            base::m[i * M + j] = static_cast<T>(other(i, j));

    // Zero-fill the rest
    for (usize k = rows_to_copy * M; k < N * M; ++k)
        base::mat[k] = T(0);
    for (usize i = 0; i < rows_to_copy; ++i)
        for (usize j = cols_to_copy; j < M; ++j)
            base::mat[i * M + j] = T(0);
}

// Assign from a matrix (min copy, zero-fill tail).
template <typename T, usize N, usize M>
template <convertible_to<T> U, usize K, usize L>
constexpr Matrix<T, N, M>& Matrix<T, N, M>::operator=(const Matrix<U, K, L>& rhs) noexcept {
    const usize rows_to_copy = (N < K) ? N : K;
    const usize cols_to_copy = (M < L) ? M : L;

    // Copy the overlapping region
    for (usize i = 0; i < rows_to_copy; ++i)
        for (usize j = 0; j < cols_to_copy; ++j)
            base::m[i * M + j] = static_cast<T>(rhs(i, j));

    // Zero-fill the rest
    for (usize k = rows_to_copy * M; k < N * M; ++k)
        base::mat[k] = T(0);
    for (usize i = 0; i < rows_to_copy; ++i)
        for (usize j = cols_to_copy; j < M; ++j)
            base::mat[i * M + j] = T(0);

    return *this;
}

}  // namespace nme::math