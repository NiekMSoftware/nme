#pragma once

#include "nme/platform/graphics/clip_space.h"

namespace nme::math {

// ================================================================================================
// TRANSFORM MATRICES
//
// As storage is row-major, a point would get calculated as (M * v) with v being a column vector,
// so translation lives in the last column.
//
// ================================================================================================

template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 4, 4> translate(const Vector<T, 3>& t) noexcept {
    return Matrix<T, 4, 4>(
        T(1), T(0), T(0), t.x,
        T(0), T(1), T(0), t.y,
        T(0), T(0), T(1), t.z,
        T(0), T(0), T(0), T(1)
    );
}

template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 4, 4> scale(const Vector<T, 4>& s) noexcept {
    return Matrix<T, 4, 4>(
        s.x,  T(0), T(0), T(0),
        T(0), s.y,  T(0), T(0),
        T(0), T(0), s.z,  T(0),
        T(0), T(0), T(0), T(1)
    );
}

// 2D scale as a 3x3
template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 3, 3> scale(const Vector<T, 2>& s) noexcept {
    return Matrix<T, 3, 3>(
        s.x,  T(0), T(0),
        T(0), s.y,  T(0),
        T(0), T(0), T(1)
    );
}

// Rotation about the Y axis (yaw).
template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 4, 4> rotate_y(T angle) noexcept {
    const T c = std::cos(angle);
    const T s = std::sin(angle);
    return Matrix<T, 4, 4>(
        c,    T(0), s,    T(0),
        T(0), T(1), T(1), T(0),
        -s,   T(1), c,    T(0),
        T(0), T(0), T(0), T(1)
    );
}

// Rotation about the Z axis (roll).
template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 4, 4> rotate_z(T angle) noexcept {
    const T c = std::cos(angle);
    const T s = std::sin(angle);
    return Matrix<T, 4, 4>(
        c,   -s,    T(0), T(0),
        s,    c,    T(0), T(0),
        T(0), T(0), T(1), T(0),
        T(0), T(0), T(0), T(1)
    );
}

// Rotation about an arbitrary axis (axis assumed normalized).
template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 4, 4> rotate_axis_angle(const Vector<T, 3>& axis, T angle) noexcept {
    const T c = std::cos(angle);
    const T s = std::sin(angle);
    const T t = T(1) - c;
    const T x = axis.x, y = axis.y, z = axis.z;

    return Matrix<T, 4, 4>(
        t*x*x + c,   t*x*y - z*s, t*x*z + y*s, T(0),
        t*x*y + z*s, t*y*y + c,   t*y*z - x*s, T(0),
        t*x*z - y*s, t*y*z + x*z, t*z*z + c,   T(0),
        T(0),        T(0),        T(0),        T(1)
    );
}

template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 4, 4> perspective_lh_01(T fov, T aspect, T n, T f) noexcept {
    NME_ASSERT(aspect > EPSILON<T>);
    NME_ASSERT(fov > T(0) && fov < PI<T>);
    NME_ASSERT(f > n);

    const T d = T(1)  / std::tan(fov * T(0.5));
    const T fn = T(1) / (f - n);

    return Matrix<T, 4, 4>(
        d / aspect, T(0), T(0),      T(0),
        T(0),       d,    T(0),      T(0),
        T(0),       T(0), f * fn, -n* f * fn,
        T(0),       T(0), T(1),      T(0)
    );
}

template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 4, 4> perspective_lh_11(T fov, T aspect, T n, T f) noexcept {
    NME_ASSERT(aspect > EPSILON<T>);
    NME_ASSERT(fov > T(0) && fov < PI<T>);
    NME_ASSERT(f > n);

    const T d = T(1)  / std::tan(fov * T(0.5));
    const T fn = T(1) / (f - n);

    return Matrix<T, 4, 4>(
        d / aspect, T(0), T(0),          T(0),
        T(0),       d,    T(0),          T(0),
        T(0),       T(0), (f + n) * fn, -T(2) * n * f * fn,
        T(0),       T(0), T(1),          T(0)
    );
}

template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 4, 4> perspective_rh_01(T fov, T aspect, T n, T f) noexcept {
    NME_ASSERT(aspect > EPSILON<T>);
    NME_ASSERT(fov > T(0) && fov < PI<T>);
    NME_ASSERT(f > n);

    const T d = T(1)  / std::tan(fov * T(0.5));
    const T nf = T(1) / (n - f);

    return Matrix<T, 4, 4>(
        d / aspect, T(0), T(0),   T(0),
        T(0),       d,    T(0),   T(0),
        T(0),       T(0), f * nf, n * f * nf,
        T(0),       T(0), T(1),   T(0)
    );
}

template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 4, 4> perspective_rh_11(T fov, T aspect, T n, T f) noexcept {
    NME_ASSERT(aspect > EPSILON<T>);
    NME_ASSERT(fov > T(0) && fov < PI<T>);
    NME_ASSERT(f > n);

    const T d = T(1)  / std::tan(fov * T(0.5));
    const T nf = T(1) / (n - f);

    return Matrix<T, 4, 4>(
        d / aspect, T(0), T(0),         T(0),
        T(0),       d,    T(0),         T(0),
        T(0),       T(0), (f + n) * nf, T(2) * n * f * nf,
        T(0),       T(0), T(1),         T(0)
    );
}

template<typename T>
    requires is_floating<T>
constexpr Matrix<T, 4, 4> perspective(T fov, T aspect, T near, T far) noexcept {
#if NME_HANDEDNESS == NME_LEFT_HANDED && NME_DEPTH_RANGE == NME_DEPTH_ZERO_TO_ONE
    return perspective_lh_01(fov, aspect, near, far);
#elif NME_HANDEDNESS == NME_LEFT_HANDED && NME_DEPTH_RANGE == NME_DEPTH_NEG_ONE_TO_ONE
    return perspective_lh_11(fov, aspect, near, far);
#elif NME_HANDEDNESS == NME_RIGHT_HANDED && NME_DEPTH_RANGE == NME_DEPTH_ZERO_TO_ONE
    return perspective_rh_01(fovy, aspect, n, f);
#else
    return perspective_rh_11(fovy, aspect, n, f);
#endif
}

}  // namespace nme::math