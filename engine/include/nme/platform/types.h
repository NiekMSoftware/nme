#pragma once
//==============================================================================
// Platform Independence Layer -- Primitive Data Types  (Gregory 4e, sec. 1.5)
//------------------------------------------------------------------------------
// Fixed-width aliases. Sized types are a platform-layer concern: their
// guarantees let the rest of the engine reason about layout/serialization/SIMD
// identically across compilers and CPUs.
//==============================================================================

#include "nme/platform/platform.h"

#include <cstddef>
#include <cstdint>

namespace nme {

typedef std::int8_t  i8;
typedef std::int16_t i16;
typedef std::int32_t i32;
typedef std::int64_t i64;

using int8 = i8;
using int16 = i16;
using int32 = i32;
using int64 = i64;

typedef std::uint8_t  u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;

using uint8 = u8;
using uint16 = u16;
using uint32 = u32;
using uint64 = u64;

typedef std::size_t    usize;
typedef std::ptrdiff_t ptrdiff_type;

typedef std::intptr_t  iptr;
typedef std::uintptr_t uptr;

typedef float  f32;
typedef double f64;

using float32 = float;
using float64 = double;

typedef u8  b8;      ///< Natural choice for packed flag fields where each byte counts.
typedef i32 b32;     ///< Matches most different ABIs their BOOL32 flags.

static_assert(sizeof(i8) == 1, "NME: i8 must be 1 byte");
static_assert(sizeof(i16) == 2, "NME: i16 must be 2 bytes");
static_assert(sizeof(i32) == 4, "NME: i32 must be 4 bytes");
static_assert(sizeof(i64) == 8, "NME: i64 must be 8 bytes");

static_assert(sizeof(u8) == 1, "NME: u8 must be 1 byte");
static_assert(sizeof(u16) == 2, "NME: u16 must be 2 bytes");
static_assert(sizeof(u32) == 4, "NME: u32 must be 4 bytes");
static_assert(sizeof(u64) == 8, "NME: u64 must be 8 bytes");

static_assert(sizeof(f32) == 4, "NME: f32 must be 4 bytes");
static_assert(sizeof(f64) == 8, "NME: f64 must be 8 bytes");

static_assert(sizeof(usize) == NME_PTR_SIZE, "NME: usize must match pointer size");
static_assert(sizeof(ptrdiff_type) == NME_PTR_SIZE, "NME: ptrdiff_type must match pointer size");
static_assert(sizeof(iptr) == NME_PTR_SIZE, "NME: iptr must match pointer size");
static_assert(sizeof(uptr) == NME_PTR_SIZE, "NME: uptr must match pointer size");
static_assert(sizeof(void *) == NME_PTR_SIZE, "NME: pointer size disagrees with NME_PTR_SIZE");

static_assert(sizeof(b32) == 4, "NME: Bool32 must be 4 bytes");

}  // namespace nme