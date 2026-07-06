#pragma once

namespace nme::math {
/** @brief Base class of all vectors. */
template<typename T, usize N>
struct VectorBase {
    constexpr VectorBase() noexcept;

    T data[N];
};

template <typename T, usize N>
constexpr VectorBase<T, N>::VectorBase() noexcept
    : data{}
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
        T data[2];
    };
};

template <typename T>
constexpr VectorBase<T, 2>::VectorBase() noexcept
    : data{}
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
        T data[3];
    };
};

template <typename T>
constexpr VectorBase<T, 3>::VectorBase() noexcept
: data{}
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
        T data[4];
    };
};

template <typename T>
constexpr VectorBase<T, 4>::VectorBase() noexcept
: data{}
{ }

#if defined(NME_SSE)

// TODO: Create specialization for 4-component floating-point vector with SSE

#endif

}  // namespace nme::math