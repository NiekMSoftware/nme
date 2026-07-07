#pragma once

namespace nme::math {

namespace detail {

template<typename T, usize N, usize M, typename F>
constexpr Matrix<T, N, M> map(const Matrix<T, N, M>& a, F f) noexcept {
    Matrix<T, N, M> r;
    for (usize i = 0; i < N * M; ++i) r.data()[i] = f(a.data()[i]);
    return r;
}

template<typename T, usize N, usize M, typename F>
constexpr Matrix<T, N, M> zip(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b, F f) noexcept {
    Matrix<T, N, M> r;
    for (usize i = 0; i < N * M; ++i) r.data()[i] = f(a.data()[i], b.data()[i]);
    return r;
}

}  // namespace detail

// -------------------------------- unary -------------------------------------

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> operator+(const Matrix<T, N, M>& m) noexcept {
    return m;
}

template<typename T, usize N, usize M>
requires is_signed<T>
constexpr Matrix<T, N, M> operator-(const Matrix<T, N, M>& m) noexcept {
    return detail::map(m, [](T x) { return -x; });
}

// --------------------------- matrix <op> matrix -----------------------------

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> operator+(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x + y; });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> operator-(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x - y; });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> operator*(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x * y; });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> operator/(const Matrix<T, N, M>& a, const Matrix<T, N, M> b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x / y; });
}

}  // namespace nme::math