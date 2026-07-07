#pragma once

#include "nme/platform/platform.h"
#include "nme/platform/types.h"

namespace nme::math {

/** @brief Base class of all vectors. */
template<typename T, usize N>
struct VectorBase {
    constexpr VectorBase() noexcept;

    T vec[N];
};

template <typename T, usize N>
constexpr VectorBase<T, N>::VectorBase() noexcept
    : vec{}
{ }

/** @brief Partial specialization for 2-component vectors. */
template<typename T>
struct VectorBase<T, 2> {
    constexpr VectorBase() noexcept;

    union {
        struct {
            T x, y;
        };
        struct {
            T r, g;
        };
        struct {
            T s, t;
        };
        struct {
            T u, v;
        };
        T vec[2];
    };
};

template <typename T>
constexpr VectorBase<T, 2>::VectorBase() noexcept
    : vec{}
{ }

/** @brief Partial specialization for 3-component vectors. */
template<typename T>
struct VectorBase<T, 3> {
    constexpr VectorBase() noexcept;

    union {
        struct {
            T x, y, z;
        };
        struct {
            T r, g, b;
        };
        T vec[3];
    };
};

template <typename T>
constexpr VectorBase<T, 3>::VectorBase() noexcept
    : vec{}
{ }

/** @brief Partial specialization for 4-component vectors. */
template<typename T>
struct VectorBase<T, 4> {
    constexpr VectorBase() noexcept;

    union {
        struct {
            T x, y, z, w;
        };
        struct {
            T r, g, b, a;
        };
        T vec[4];
    };
};

template <typename T>
constexpr VectorBase<T, 4>::VectorBase() noexcept
    : vec{}
{ }

#if defined(NME_SIMD_SSE)

template<>
struct alignas(16) VectorBase<f32, 4> {
    constexpr VectorBase() noexcept;
    explicit constexpr VectorBase(__m128 v) noexcept;

    union {
        struct {
            f32 x, y, z, w;
        };
        struct {
            f32 r, g, b, a;
        };
        f32 vec[4];
        __m128 v;
    };

    explicit constexpr operator __m128() const noexcept;
};
constexpr VectorBase<float, 4>::VectorBase() noexcept
    : vec {}
{ }
constexpr VectorBase<float, 4>::VectorBase(__m128 v) noexcept
    : v {v}
{ }

constexpr VectorBase<float, 4>::operator __m128() const noexcept { return v; }

#endif

}  // namespace nme::math