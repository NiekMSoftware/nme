// Unit tests for nme::math vectors.
// Covers storage/aliasing, constructors, all operators (incl. bitwise on
// integers), geometric ops, the free-function set, and GLM parity.
//
// Unit tests written by a LLM to fully cover everything.

#include <doctest/doctest.h>

#include <random>
#include <type_traits>

#include <nme/core/math/vec.hpp>

#ifdef NME_TEST_WITH_GLM
#include <glm/glm.hpp>
#endif

using namespace nme;
using namespace nme::math;

