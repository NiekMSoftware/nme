#pragma once

#include <type_traits>
#include "nme/platform/types.h"

namespace nme::math {

template<typename T, usize N, usize M>
struct Matrix;  // fwd

template<typename T, usize N, usize M>
struct component_count<Matrix<T, N, M>> : std::integral_constant<usize, N * M> {};

// Is `Arg` usable as a component source for element type T?
template<typename Arg, typename T>
struct is_matrix_component : std::bool_constant<convertible_to<Arg, T>> {};

template<typename U, usize N, typename T>
struct is_matrix_component<Vector<U, N>, T> :  std::bool_constant<convertible_to<U, T>> {};

template<typename U, usize N, usize M, typename T>
struct is_matrix_component<Matrix<U, N, M>, T> :  std::bool_constant<convertible_to<U, T>> {};

// Every argument in the pack is a valid component source.
template<typename T, typename... Args>
concept matrix_component_pack =
    (is_matrix_component<std::remove_cvref_t<Args>, T>::value && ...);

}  // namespace nme::math