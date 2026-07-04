#pragma once
//==============================================================================
// Platform Independence Layer -- Primitive Data Types  (Gregory 4e, sec. 1.5)
//------------------------------------------------------------------------------
// Fixed-width aliases. Sized types are a platform-layer concern: their
// guarantees let the rest of the engine reason about layout/serialization/SIMD
// identically across compilers and CPUs.
//==============================================================================

#include "nme/platform/platform.h"

namespace nme {

#if NME_COMPILER_MSVC
	typedef signed  __int8  i8;
	typedef signed  __int16 i16;
	typedef signed  __int32 i32;
	typedef signed  __int64 i64;

	typedef unsigned __int8  u8;
	typedef unsigned __int16 u16;
	typedef unsigned __int32 u32;
	typedef unsigned __int64 u64;
#elif NME_COMPILER_GCC || NME_COMPILER_CLANG
	typedef __INT8_TYPE__   i8;
	typedef __INT16_TYPE__  i16;
	typedef __INT32_TYPE__  i32;
	typedef __INT64_TYPE__  i64;

	typedef __UINT8_TYPE__  u8;
	typedef __UINT16_TYPE__ u16;
	typedef __UINT32_TYPE__ u32;
	typedef __UINT64_TYPE__ u64;
#endif

#if NME_64BIT
	typedef u64 usize;
	typedef i64 ptrdiff_type;

	typedef i64 iptr;
	typedef u64 uptr;
#elif NME_32BIT
	typedef u32 usize;
	typedef i32 ptrdiff_type;

	typedef i32 iptr;
	typedef u32 uptr;
#endif

typedef float  f32;
typedef double f64;

typedef u8 b8;		 ///< Natural choice for packed flag fields where each byte counts.
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
