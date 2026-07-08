#pragma once

#include "nme/core/math/vec.hpp"
#include "nme/platform/types.h"

namespace nme::math {

template<typename T, usize N, usize M>
struct MatrixBase {
    constexpr MatrixBase() noexcept;

    union {
        Vector<T, M> row[N];
        T            mat[N * M];
    };
};
template <typename T, usize N, usize M>
constexpr MatrixBase<T, N, M>::MatrixBase() noexcept : mat {} { }

template <typename T, usize M>
struct MatrixBase<T, 2, M> {
    constexpr MatrixBase() noexcept;

    union {
        Vector<T, M> X, Y;
    };
    Vector<T, M> row[2];
    T            mat[2 * M];
};
template <typename T, usize M>
constexpr MatrixBase<T, 2, M>::MatrixBase() noexcept : mat{} { }

template <typename T, usize M>
struct MatrixBase<T, 3, M> {
    constexpr MatrixBase() noexcept;

    union {
        Vector<T, M> X, Y, Z;
    };
    Vector<T, M> row[3];
    T            mat[3 * M];
};
template <typename T, usize M>
constexpr MatrixBase<T, 3, M>::MatrixBase() noexcept : mat{} { }

template <typename T, usize M>
struct MatrixBase<T, 4, M> {
    constexpr MatrixBase() noexcept;

    union {
        Vector<T, M> X, Y, Z, W;
    };
    Vector<T, M> row[4];
    T            mat[4 * M];
};
template <typename T, usize M>
constexpr MatrixBase<T, 4, M>::MatrixBase() noexcept : mat{} { }

}  // nme::math