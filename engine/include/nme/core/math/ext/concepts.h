#pragma once

#include <type_traits>

namespace nme::math {

template<typename T> struct type_identity { using type = T; };
template<typename T> using type_identity_t = typename type_identity<T>::type;

template<typename T> inline constexpr bool is_floating = std::is_floating_point_v<T>;
template<typename T> inline constexpr bool is_integral = std::is_integral_v<T>;

template<typename T> inline constexpr bool is_bitwise = is_integral<T> && !std::is_same_v<T, bool>;

}  // nme::math