#pragma once

// matrix.inl -- everything indexes the flat, column-major buffer: element (r,c) == data[c*Rows + r].
// that buffer aliases the `col` vectors, so results built here read back correctly
// through operator()/operator[] too

#include "../concepts.h"

namespace nme::math {

// Clip-space conventions for the projection builders
// ClipDepth::ZeroToOne   -> D3D12 / VK
// ClipDepth::NegOneToOne -> OpenGL
enum class ClipDepth  { ZeroToOne, NegOneToOne };
enum class Handedness { RightHanded, LeftHanded };

namespace detail {
template<typename T> constexpr T det2(T a, T b, T c, T d) { return a*d - b*c; }
template<typename T> constexpr T det3(T a, T b, T c, T d, T e, T f, T g, T h, T i) {
    return a*(e*i - f*h) - b*(d*i - f*g) + c*(d*h - e*g);
}
}  // namespace  detail

// ---------------------------------------------
//                   Products
// ---------------------------------------------

// Matrix * Matrix (inner dimension N cancels): (R x N) * (N x C) -> (R x C)
template<typename T, u32 R, u32 N, u32 C>
constexpr Matrix<T, R, C> operator*(const Matrix<T, R, N>& a, const Matrix<T, N, C>& b) {
    Matrix<T, R, C> out{};
    for (u32 c = 0; c < C; ++c)
        for (u32 k = 0; k < N; ++k)
            for (u32 r = 0; r < R; ++r)
                out.data[c*R + r] += a.data[k*R + r] * b.data[c*N + k];
    return out;
}

// Matrix * column-vector: (R x C) * (C) -> (R)
template<typename T, u32 R, u32 C>
constexpr Vector<T, R> operator*(const Matrix<T, R, C>& m, const Vector<T, C>& v) {
    Vector<T, R> out{};
    for (u32 c = 0; c < C; ++c)
        for (u32 r = 0; r < R; ++r)
            out.data[r] += m.data[c*R + r] * v.data[c];
    return out;
}

template<typename T, u32 N>
constexpr Matrix<T, N, N>& operator*=(Matrix<T, N, N>& a, const Matrix<T, N, N>& b) {
    a = a * b;
    return a;
}

// ---------------------------------------------
//             Arithmetic operators
// ---------------------------------------------

template<typename T, u32 R, u32 C>
constexpr Matrix<T, R, C> operator-(Matrix<T, R, C> m) {
    for (u32 i = 0; i < R * C; ++i) m.data[i] = -m.data[i];
    return m;
}

template<typename T, u32 R, u32 C>
constexpr Matrix<T, R, C> operator+(Matrix<T, R, C> a, const Matrix<T, R, C>& b) {
    for (u32 i = 0; i < R * C; ++i) a.data[i] += b.data[i];
    return a;
}
template<typename T, u32 R, u32 C>
constexpr Matrix<T, R, C> operator-(Matrix<T, R, C> a, const Matrix<T, R, C>& b) {
    for (u32 i = 0; i < R * C; ++i) a.data[i] -= b.data[i];
    return a;
}

template<typename T, u32 R, u32 C>
constexpr Matrix<T, R, C> operator*(Matrix<T, R, C> m, detail::type_identity_t<T> s) {
    for (u32 i = 0; i < R * C; ++i) m.data[i] *= s;
    return m;
}
template<typename T, u32 R, u32 C>
constexpr Matrix<T, R, C> operator*(detail::type_identity_t<T> s, Matrix<T, R, C> m) {
    return m * s;
}
template<typename T, u32 R, u32 C>
constexpr Matrix<T, R, C> operator/(Matrix<T, R, C> m, detail::type_identity_t<T> s) {
    for (u32 i = 0; i < R * C; ++i) m.data[i] /= s;
    return m;
}

template<typename T, u32 R, u32 C>
constexpr Matrix<T, R, C>& operator+=(Matrix<T, R, C>& a, const Matrix<T, R, C>& b) {
    for (u32 i = 0; i < R * C; ++i) a.data[i] += b.data[i];
    return a;
}
template<typename T, u32 R, u32 C>
constexpr Matrix<T, R, C>& operator-=(Matrix<T, R, C>& a, const Matrix<T, R, C>& b) {
    for (u32 i = 0; i < R * C; ++i) a.data[i] -= b.data[i];
    return a;
}
template<typename T, u32 R, u32 C>
constexpr Matrix<T, R, C>& operator*=(Matrix<T, R, C>& m, detail::type_identity_t<T> s) {
    for (u32 i = 0; i < R * C; ++i) m.data[i] *= s;
    return m;
}
template<typename T, u32 R, u32 C>
constexpr Matrix<T, R, C>& operator/=(Matrix<T, R, C>& m, detail::type_identity_t<T> s) {
    for (u32 i = 0; i < R * C; ++i) m.data[i] /= s;
    return m;
}

// ---------------------------------------------
//                  Comparison
// ---------------------------------------------

template<typename T, u32 R, u32 C>
bool operator==(const Matrix<T, R, C>& a, const Matrix<T, R, C>& b) {
    for (u32 i = 0; i < R * C; ++i)
        if (!(a.data[i] == b.data[i])) return false;
    return true;
}
template<typename T, u32 R, u32 C>
bool operator!=(const Matrix<T, R, C>& a, const Matrix<T, R, C>& b) { return !(a == b); }

template<typename T, u32 R, u32 C>
[[nodiscard]] constexpr std::enable_if_t<detail::is_floating<T>, bool>
is_near(const Matrix<T, R, C>& a, const Matrix<T, R, C>& b, detail::type_identity_t<T> eps = T(1e-5)) {
    for (u32 i = 0; i < R * C; ++i) {
        T d = a.data[i] - b.data[i];
        if ((d < T{0} ? -d : d) > eps) return false;
    }
    return true;
}

// ---------------------------------------------
//                  Utilities
// ---------------------------------------------

// (R x C) -> (C x R)
template<typename T, u32 R, u32 C>
[[nodiscard]] constexpr Matrix<T, C, R> transpose(const Matrix<T, R, C>& m) {
    Matrix<T, C, R> out{};
    for (u32 i = 0; i < R; ++i)
        for (u32 j = 0; j < C; ++j)
            out.data[i*C + j] = m.data[j*R + i];
    return out;
}

// identity<f32, 4>(), or Matrix(T{1}) via the diagonal constructor.
template<typename T, u32 N>
[[nodiscard]] constexpr Matrix<T, N, N> identity() {
    Matrix<T, N, N> out{};
    for (u32 i = 0; i < N; ++i) out.data[i*N + i] = T{1};
    return out;
}

template<typename T, u32 N>
[[nodiscard]] constexpr T trace(const Matrix<T, N, N>& m) {
    T s{};
    for (u32 i = 0; i < N; ++i) s += m.data[i*N + i];
    return s;
}

template<typename T, u32 N>
[[nodiscard]] constexpr Vector<T, N> diagonal(const Matrix<T, N, N>& m) {
    Vector<T, N> v{};
    for (u32 i = 0; i < N; ++i) v.data[i] = m.data[i*N + i];
    return v;
}

template<typename T, u32 R, u32 C>
[[nodiscard]] constexpr Vector<T, R> column(const Matrix<T, R, C>& m, const u32 c) {
    Vector<T, R> v{};
    for (u32 r = 0; r < R; ++r) v.data[r] = m.data[c*R + r];
    return v;
}
template<typename T, u32 R, u32 C>
[[nodiscard]] constexpr Vector<T, R> row(const Matrix<T, R, C>& m, const u32 r) {
    Vector<T, R> v{};
    for (u32 c = 0; c < C; ++c) v.data[c] = m.data[c*R + r];
    return v;
}
template<typename T, u32 R, u32 C>
constexpr void set_column(Matrix<T, R, C>& m, const u32 c, const Vector<T, R>& v) {
    for (u32 r = 0; r < R; ++r) m.data[c*R + r] = v.data[r];
}
template<typename T, u32 R, u32 C>
constexpr void set_row(Matrix<T, R, C>& m, const u32 r, const Vector<T, R>& v) {
    for (u32 c = 0; c < C; ++c) m.data[c*R + r] = v.data[c];
}

// embed a 3x3 into the upper-left of a 4x4 (translation zero)
template<typename T>
[[nodiscard]] constexpr Matrix<T, 4, 4> to_mat4(const Matrix<T, 3, 3>& m) {
    Matrix<T, 4, 4> r(T{1});
    for (u32 c = 0; c < 3; ++c)
        for (u32 rr = 0; rr < 3; ++rr) r.data[c*4 + rr] = m.data[c*3 + rr];
    return r;
}

// embed a 4x4 into the upper left of a 3x3
template<typename T>
[[nodiscard]] constexpr Matrix<T, 3, 3> to_mat3(const Matrix<T, 4, 4>& m) {
    Matrix<T, 3, 3> r(T{1});
    for (u32 c = 0; c < 3; ++c)
        for (u32 rr = 0; rr < 3; ++rr) r.data[c*3 + rr] = m.data[c*4 + rr];
    return r;
}

// ---------------------------------------------
//          Determinant & inverse
// ---------------------------------------------

template<typename T>
[[nodiscard]] constexpr T determinant(const Matrix<T, 2, 2>& m) {
    return detail::det2(m(0,0), m(0,1), m(1,0), m(1,1));
}
template<typename T>
[[nodiscard]] constexpr T determinant(const Matrix<T, 3, 3>& m) {
    return detail::det3(m(0,0), m(0,1), m(0,2),
                        m(1,0), m(1,1), m(1,2),
                        m(2,0), m(2,1), m(2,2));
}
template<typename T>
[[nodiscard]] constexpr T determinant(const Matrix<T, 4, 4>& m) {
    T d{};
    for (u32 c = 0; c < 4; ++c) {
        u32 ci[3], q = 0;
        for (u32 k = 0; k < 4; ++k) if (k != c) ci[q++] = k;
        T minor = detail::det3(m(1,ci[0]), m(1,ci[1]), m(1,ci[2]),
                               m(2,ci[0]), m(2,ci[1]), m(2,ci[2]),
                               m(3,ci[0]), m(3,ci[1]), m(3,ci[2]));
        d += ((c & 1u) ? -m(0,c) : m(0,c)) * minor;
    }
    return d;
}

template<typename T>
[[nodiscard]] constexpr std::enable_if_t<detail::is_floating<T>, Matrix<T, 2, 2>>
inverse(const Matrix<T, 2, 2>& m) {
    T inv_det = T{1} / determinant(m);
    Matrix<T, 2, 2> r{};
    r(0,0) =  m(1,1) * inv_det; r(0,1) = -m(0,1) * inv_det;
    r(1,0) = -m(1,0) * inv_det; r(1,1) =  m(0,0) * inv_det;
    return r;
}

template<typename T>
[[nodiscard]] constexpr std::enable_if_t<detail::is_floating<T>, Matrix<T, 3, 3>>
inverse(const Matrix<T, 3, 3>& m) {
    Matrix<T, 3, 3> adj{};
    for (u32 c = 0; c < 3; ++c) for (u32 r = 0; r < 3; ++r) {
        u32 ri[2], ci[2], p = 0, q = 0;
        for (u32 k = 0; k < 3; ++k) if (k != r) ri[p++] = k;
        for (u32 k = 0; k < 3; ++k) if (k != c) ci[q++] = k;
         T minor = detail::det2(m(ri[0],ci[0]), m(ri[0],ci[1]),
                                   m(ri[1],ci[0]), m(ri[1],ci[1]));
            adj(c,r) = ((r + c) & 1u) ? -minor : minor;   // transposed cofactor
    }
    T inv_det = T{1} / (m(0,0)*adj(0,0) + m(0,1)*adj(1,0) + m(0,2)*adj(2,0));
    for (u32 i = 0; i < 9; ++i) adj.data[i] *= inv_det;
    return adj;
}

template<typename T>
[[nodiscard]] constexpr std::enable_if_t<detail::is_floating<T>, Matrix<T, 4, 4>>
inverse(const Matrix<T, 4, 4>& m) {
    Matrix<T, 4, 4> adj{};
    for (u32 c = 0; c < 4; ++c) for (u32 r = 0; r < 4; ++r) {
        u32 ri[3], ci[3], p = 0, q = 0;
        for (u32 k = 0; k < 4; ++k) if (k != r) ri[p++] = k;
        for (u32 k = 0; k < 4; ++k) if (k != c) ci[q++] = k;
        T minor = detail::det2(m(ri[0],ci[0]), m(ri[0],ci[1]), m(ri[0],ci[2]),
                                m(ri[1], ci[0]), m(ri[1],ci[1]), m(ri[1],ci[2]),
                                m(ri[2], ci[0]), m(ri[2], ci[1]), m(ri[2], ci[2]));
        adj(c,r) = ((r + c) & 1u) ? -minor : minor;     // transposed cofactor
    }
    T inv_det = T{1} / (m(0,0)*adj(0,0) + m(0,1)*adj(1,0) + m(0,2)*adj(2,0) + m(0,3)*adj(3,0));
    for (u32 i = 0; i < 16; ++i) adj.data[i] *= inv_det;
    return adj;
}

// Fast inverse for an affine 4x4 (last row 0,0,0,1): invert the linear 3x3,
// then re-derive the translation. Handles rotation, scale and shear.
template<typename T>
[[nodiscard]] std::enable_if_t<detail::is_floating<T>, Matrix<T, 4, 4>>
inverse_affine(const Matrix<T, 4, 4>& m) {
    Matrix<T, 3, 3> a = inverse(to_mat3(m));
    Vector<T, 3> t{ m(0,3), m(1,3), m(2,3) };
    Vector<T, 3> nt = -(a * t);
    Matrix<T, 4, 4> r = to_mat4(a);
    r(0,3) = nt.x; r(1,3) = nt.y; r(2,3) = nt.z;
    return r;
}

// ---------------------------------------------
//                  Transforms
// ---------------------------------------------

template<typename T>
[[nodiscard]] constexpr Matrix<T, 4, 4> translation(const Vector<T, 3>& t) {
    Matrix<T, 4, 4> m(T{1});
    m(0,3) = t.x; m(1,3) = t.y; m(2,3) = t.z;
    return m;
}

template<typename T>
[[nodiscard]] constexpr Matrix<T, 4, 4> scale(const Vector<T, 3>& s) {
    Matrix<T, 4, 4> m(T{1});
    m(0,0) = s.x; m(1,1) = s.y; m(2,2) = s.z;
    return m;
}
template<typename T>
[[nodiscard]] constexpr Matrix<T, 4, 4> scale(T s) {
    return scale(Vector<T, 3>(s, s, s));
}

// axis assumed unit length
template<typename T>
[[nodiscard]] constexpr std::enable_if_t<detail::is_floating<T>, Matrix<T, 4, 4>>
rotation(const Vector<T, 3>& axis, detail::type_identity_t<T> angle) {
    const T c = std::cos(angle), s = std::sin(angle), t = T{1} - c;
    const T x = axis.x, y = axis.y, z = axis.z;
    Matrix<T, 4, 4> m(T{1});
    m(0,0)  = t*x*x + c;   m(0, 1) = t*x*y - s*z; m(0, 2) = t*x*z + s*y;
    m(1,0)  = t*x*y + s*z; m(1, 1) = t*y*y + c;   m(1, 2) = t*y*z - s*x;
    m(2, 0) = t*x*z - s*y; m(2, 1) = t*y*z + s*x; m(2, 2) = t*z*z + c;
    return m;
}

}  // namespace nme::math

// TODO: look_at, and perspective/ortho.
// The projections need the per-API depth range
// (D3D12 & Vulkan z in [0,1], OpenGL[-1,1]) and Vulkan's Y-flip, so they get built
// per-API rather than as one factory (maybe, unsure yet on that design).