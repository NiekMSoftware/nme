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

// --------------------------- matrix +/- matrix -----------------------------
// (Matrix addition/subtraction is element-wise, so these are the real ops.)

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> operator+(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x + y; });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> operator-(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x - y; });
}

// ------------------------- component-wise products --------------------------
// The Hadamard (element-wise) product/quotient are NOT operator*/operator/,
// because operator* is reserved for the matrix product below (cf. GLM's
// matrixCompMult). Named functions keep both available without ambiguity.

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> comp_mul(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x * y; });
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M> comp_div(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    return detail::zip(a, b, [](T x, T y) { return x / y; });
}

// --------------------------- matrix +=/-= matrix ----------------------------

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M>& operator+=(Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    for (usize i = 0; i < N * M; ++i) a.data()[i] += b.data()[i];
    return a;
}

template<typename T, usize N, usize M>
constexpr Matrix<T, N, M>& operator-=(Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    for (usize i = 0; i < N * M; ++i) a.data()[i] -= b.data()[i];
    return a;
}

// -------------------- matrix <op> scalar (broadcast) -----------------------

template<typename T, usize N, usize M, convertible_to<T> U>
constexpr Matrix<T, N, M> operator*(const Matrix<T, N, M>& m, U s) noexcept {
    const T t = static_cast<T>(s);
    return detail::map(m, [t](T x) { return x * t; });
}

template<typename T, usize N, usize M, convertible_to<T> U>
constexpr Matrix<T, N, M> operator*(U s, const Matrix<T, N, M>& m) noexcept {
    return m * s;  // commutative
}

template<typename T, usize N, usize M, convertible_to<T> U>
constexpr Matrix<T, N, M> operator/(const Matrix<T, N, M>& m, U s) noexcept {
    NME_ASSERT(static_cast<T>(s) != T(0));
    const T t = static_cast<T>(s);
    return detail::map(m, [t](T x) { return x / t; });
}

template<typename T, usize N, usize M, convertible_to<T> U>
constexpr Matrix<T, N, M>& operator*=(Matrix<T, N, M>& m, U s) noexcept {
    const T t = static_cast<T>(s);
    for (usize i = 0; i < N * M; ++i) m.data()[i] *= t;
    return m;
}

template<typename T, usize N, usize M, convertible_to<T> U>
constexpr Matrix<T, N, M>& operator/=(Matrix<T, N, M>& m, U s) noexcept {
    NME_ASSERT(static_cast<T>(s) != T(0));
    const T t = static_cast<T>(s);
    for (usize i = 0; i < N * M; ++i) m.data()[i] /= t;
    return m;
}

// ---------------------------- matrix product ------------------------------

// Matrix * Matrix: (N x M) * (M x P) -> (N x P)
template<typename T, usize N, usize M, usize P>
constexpr Matrix<T, N, P> operator*(const Matrix<T, N, M>& a, const Matrix<T, M, P>& b) noexcept {
    Matrix<T, N, P> r{};
    for (usize i = 0; i < N; ++i)
        for (usize j = 0; j < P; ++j)
            for (usize k = 0; k < M; ++k)
                r(i, j) += a(i, k) * b(k, j);
    return r;
}

// Treats vector as a column vector. Result is the transformed vector.
template<typename T, usize N, usize M>
constexpr Vector<T, N> operator*(const Matrix<T, N, M>& m, const Vector<T, M>& v) noexcept {
    Vector<T, N> r{};
    for (usize i = 0; i < N; ++i)
        for (usize j = 0; j < M; ++j)
            r[i] += m(i, j) * v[j];
    return r;
}

// Treats vector as a row vector. Result is the transformed vector.
template<typename T, usize N, usize M>
constexpr Vector<T, M> operator*(const Vector<T, N>& v, const Matrix<T, N, M>& m) noexcept {
    Vector<T, M> r{};
    for (usize i = 0; i < N; ++i)
        for (usize j = 0; j < M; ++j)
            r[j] += v[i] * m(i, j);
    return r;
}

// Matrix *= Matrix: composes in place; right operand must be square (M x M).
template<typename T, usize N, usize M>
constexpr Matrix<T, N, M>& operator*=(Matrix<T, N, M>& a, const Matrix<T, M, M>& b) noexcept {
    a = a * b;
    return a;
}

// ------------------------------ comparison ----------------------------------

template<typename T, usize N, usize M>
constexpr bool operator==(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    for (usize i = 0; i < N * M; ++i)
        if (!(a.data()[i] == b.data()[i])) return false;
    return true;
}

template<typename T, usize N, usize M>
constexpr auto operator<=>(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b) noexcept {
    for (usize i = 0; i < N * M; ++i)
        if (auto c = a.data()[i] <=> b.data()[i]; c != 0) return c;
    return a.data()[N * M - 1] <=> b.data()[N * M - 1];
}

}  // namespace nme::math
