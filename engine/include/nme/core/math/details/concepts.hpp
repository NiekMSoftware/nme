#pragma once

#include <type_traits>

namespace nme::math {

template<typename From, typename To>
concept convertible_to = std::convertible_to<From, To>;

template<typename T> struct type_identity { using type = T; };
template<typename T> using type_identity_t = typename type_identity<T>::type;

template<typename T> concept is_floating = std::floating_point<T>;
template<typename T> concept is_integral  = std::integral<T>;
template<typename T> concept is_bitwise   = std::integral<T> && !std::same_as<T, bool>;

}  // nme::math