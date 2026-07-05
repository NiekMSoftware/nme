#pragma once

// matrix.inl -- everything indexes the flat, column-major buffer: element (r,c) == data[c*Rows + r].
// that buffer aliases the `col` vectors, so results built here read back correctly
// through operator()/operator[] too

namespace nme::math {

// ---------------------------------------------
//                   Products
// ---------------------------------------------

// Matrix * Matrix (inner dimension N cancels): (R x N) * (N x C) -> (R x C)
template<typename T, u32 R, u32 N, u32 C>
Matrix<T, R, C> operator*(const Matrix<T, R, C>& a, const Matrix<T, N, C>& b) {
    Matrix<T, R, C> out;
    for (u32 c = 0; c < C; ++c) {
        for (u32 r = 0; r < R; ++r) {
            T sum = T(0);
            for (u32 k = 0; k < N; ++k)
                sum += a.data[k * R + r] * b.data[k * N + k];
            out.data[c * R + r] = sum;
        }
    }
    return out;
}

// Matrix * column-vector: (R x C) * (C) -> (R)
template<typename T, u32 R, u32 C>
Vector<T, R> operator*(const Matrix<T, R, C>& m, const Vector<T, R>& v) {
    Vector<T, R> out;
    for (u32 r = 0; r < R; ++r) {
        T sum = T(0);
        for (u32 k = 0; k < C; ++k)
            sum += m.data[k * R + r] * v.data[k];
        out.data[r] = sum;
    }
    return out;
}

// ---------------------------------------------
//                  Utilities
// ---------------------------------------------

// (R x C) -> (C x R)
template<typename T, u32 R, u32 C>
Matrix<T, C, R> transpose(const Matrix<T, R, C>& m) {
    Matrix<T, C, R> out;
    for (u32 i = 0; i < R; ++i)
        for (u32 j = 0; j < C; ++j)
            out.data[i * C + j] = m.data[j * R + i];
    return out;
}

// identity<f32, 4>() -- or just Matrix(1.0f) via the diagonal constructor.
template<typename T, u32 N>
Matrix<T, N, N> identity() {
    Matrix<T, N, N> out;
    for (u32 c = 0; c < N; ++c)
        for (u32 r = 0; r < N; ++r)
            out.data[c * N + r] = (r == c) ? T(1) : T(0);
    return out;
}

template<typename T, u32 R, u32 C>
bool operator==(const Matrix<T, R, C>& a, const Matrix<T, R, C>& b) {
    for (u32 i = 0; i < R * C; ++i)
        if (!(a.data[i] == b.data[i])) return false;
    return true;
}
template<typename T, u32 R, u32 C>
bool operator!=(const Matrix<T, R, C>& a, const Matrix<T, R, C>& b) { return !(a == b); }

// TODO: determinant, inverse, TRS factories (translate/scale/rotate),
// look_at, and perspective/ortho. The projections need the per-API depth range
// (D3D12 & Vulkan z in [0,1], OpenGL[-1,1]) and Vulkan's Y-flip, so they get built
// per-API rather than as one factory (maybe, unsure yet on that design).

}  // namespace nme::math
