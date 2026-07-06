#pragma once

namespace nme::math {

// ---- generic zero / one (all T, N) ----
template<typename T, usize N>
const Vector<T, N> VectorConstants<T, N>::zero { T(0) };

template<typename T, usize N>
const Vector<T, N> VectorConstants<T, N>::one { T(1) };

// ---- N == 2 directions ----
template<typename T> requires is_floating<T>
const Vector<T, 2> VectorConstants<T, 2>::zero  { T(0) };
template<typename T> requires is_floating<T>
const Vector<T, 2> VectorConstants<T, 2>::one   { T(1) };
template<typename T> requires is_floating<T>
const Vector<T, 2> VectorConstants<T, 2>::right {  T(1),  T(0) };
template<typename T> requires is_floating<T>
const Vector<T, 2> VectorConstants<T, 2>::left  { T(-1),  T(0) };
template<typename T> requires is_floating<T>
const Vector<T, 2> VectorConstants<T, 2>::up    {  T(0),  T(1) };
template<typename T> requires is_floating<T>
const Vector<T, 2> VectorConstants<T, 2>::down  {  T(0), T(-1) };

// ---- N == 3 directions ----
template<typename T> requires is_floating<T>
const Vector<T, 3> VectorConstants<T, 3>::zero    { T(0) };
template<typename T> requires is_floating<T>
const Vector<T, 3> VectorConstants<T, 3>::one     { T(1) };
template<typename T> requires is_floating<T>
const Vector<T, 3> VectorConstants<T, 3>::right   {  T(1),  T(0),  T(0) };
template<typename T> requires is_floating<T>
const Vector<T, 3> VectorConstants<T, 3>::left    { T(-1),  T(0),  T(0) };
template<typename T> requires is_floating<T>
const Vector<T, 3> VectorConstants<T, 3>::up      {  T(0),  T(1),  T(0) };
template<typename T> requires is_floating<T>
const Vector<T, 3> VectorConstants<T, 3>::down    {  T(0), T(-1),  T(0) };
template<typename T> requires is_floating<T>
const Vector<T, 3> VectorConstants<T, 3>::forward {  T(0),  T(0),  T(1) };
template<typename T> requires is_floating<T>
const Vector<T, 3> VectorConstants<T, 3>::back    {  T(0),  T(0), T(-1) };

}  // nme::math
