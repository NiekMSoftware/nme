#pragma once

#include <type_traits>

#include "concepts.hpp"
#include "nme/platform/types.h"

namespace nme::math {

template<typename T, usize N>
struct Vector;

// --------------------------------------------------------------------------
//  Component-pack support
//
//  A single constructor accepts any mix of scalars and smaller Vectors and
//  flattens them left-to-right. These traits answer two questions the ctor
//  needs at overload-resolution time:
//    1. "Is every argument a valid component source?"   -> vector_component_pack
//    2. "How many slots does this argument contribute?" -> component_count
// --------------------------------------------------------------------------

// How many scalar slots does one argument contribute?
//   scalar        -> 1
//   Vector<U, M>  -> M
template<typename Arg>
struct component_count : std::integral_constant<usize, 1> {};

template<typename U, usize M>
struct component_count<Vector<U, M>> : std::integral_constant<usize, M> {};

template<typename Arg>
inline constexpr usize component_count_v =
    component_count<std::remove_cvref_t<Arg>>::value;

// Is `Arg` usable as a component source for element type T?
//   - a scalar convertible to T, or
//   - a Vector whose element type is convertible to T
template<typename Arg, typename T>
struct is_vector_component : std::bool_constant<convertible_to<Arg, T>> {};

template<typename U, usize M, typename T>
struct is_vector_component<Vector<U, M>, T> : std::bool_constant<convertible_to<U, T>> {};

// Every argument in the pack is a valid component source.
template<typename T, typename... Args>
concept vector_component_pack =
    (is_vector_component<std::remove_cvref_t<Args>, T>::value && ...);

// Total slots contributed by the whole pack (used for overflow check).
template<typename... Args>
inline constexpr usize total_component_count_v = (component_count_v<Args> + ... + usize{0});

// Default tolerance for approximate comparison (equal/is_normalized)
template<typename T>
inline constexpr T EPSILON = std::numeric_limits<T>::epsilon();

// Constant for floating-point PI
template<is_floating T>
constexpr T PI = T( 3.1415926535897932384626433832795 );

// Constant for floating-point pi/2.
template<is_floating T>
constexpr T HALF_PI = PI<T> / T(2);

// Constant for floating-point PI*2
template<is_floating T>
constexpr T TWO_PI = PI<T> * T(2);

// Constant for cos(1/2), i.e. the cosine of 0.5 radians.
template<is_floating T>
constexpr T COS_ONE_OVER_TWO = T(0.87758256189037271611628158260383);

// Constant for infinity
template<has_infinity T>
constexpr T INF = std::numeric_limits<T>::infinity();

}  // namespace nme::math
