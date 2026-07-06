#pragma once

#include "./concepts.hpp"
#include "nme/platform/types.h"

namespace nme::math {

template<typename T, usize N>
struct Vector;  // fwd

template<typename T, usize N>
struct VectorConstants {
    static const Vector<T, N> zero;
    static const Vector<T, N> one;
};

template<typename T>
    requires is_floating<T>
struct VectorConstants<T, 2> {
    static const Vector<T, 2> zero;
    static const Vector<T, 2> one;

    static const Vector<T, 2> right;
    static const Vector<T, 2> left;
    static const Vector<T, 2> up;
    static const Vector<T, 2> down;
};

template<typename T>
    requires is_floating<T>
struct VectorConstants<T, 3> {
    static const Vector<T, 3> zero;
    static const Vector<T, 3> one;

    static const Vector<T, 3> right;
    static const Vector<T, 3> left;
    static const Vector<T, 3> up;
    static const Vector<T, 3> down;
    static const Vector<T, 3> forward;
    static const Vector<T, 3> back;
};

}  // namespace nme::math