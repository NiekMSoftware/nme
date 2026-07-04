/**
 * @brief Compile-time detection of the host compiler, platform, architecture,
 *        pointer size, endianness, build configuration, and required C++
 *        features.
 *
 * @file platform.h
 */

#pragma once

// ============================================================================
// Platform Detection
// ============================================================================

#if defined(_WIN32)
    #define NME_PLATFORM_WINDOWS 1
#else
    #define NME_PLATFORM_WINDOWS 0
#endif

#if defined(__linux__)
    #define NME_PLATFORM_LINUX 1
#else
    #define NME_PLATFORM_LINUX 0
#endif

#if defined(__APPLE__)
    #define NME_PLATFORM_MACOS 1
#else
    #define NME_PLATFORM_MACOS 0
#endif

#if (NME_PLATFORM_WINDOWS + NME_PLATFORM_LINUX + NME_PLATFORM_MACOS) != 1
    #error "NME: unsupported platform"
#endif

#if NME_PLATFORM_WINDOWS
    #define NME_PLATFORM_NAME "Windows"
#elif NME_PLATFORM_LINUX
    #define NME_PLATFORM_NAME "Linux"
#elif NME_PLATFORM_MACOS
    #define NME_PLATFORM_NAME "macOS"
#endif

// ============================================================================
// Compiler Detection
// ============================================================================
// Order matters. Clang on Linux also defines __GNUC__, and clang-cl on Windows
// defines both __clang__ and _MSC_VER, so __clang__ must be claimed first in
// both cases.

#if defined(__clang__)
	#define NME_COMPILER_CLANG 1
#else
	#define NME_COMPILER_CLANG 0
#endif

#if defined(_MSC_VER) && !NME_COMPILER_CLANG
	#define NME_COMPILER_MSVC 1
#else
	#define NME_COMPILER_MSVC 0
#endif

#if defined(__GNUC__) && !NME_COMPILER_CLANG
	#define NME_COMPILER_GCC 1
#else
	#define NME_COMPILER_GCC 0
#endif

#if (NME_COMPILER_CLANG + NME_COMPILER_MSVC + NME_COMPILER_GCC) != 1
	#error "NME: unsupported or ambiguous compiler"
#endif

// Compiler version
#if NME_COMPILER_CLANG
	#define NME_COMPILER_VERSION_MAJOR __clang_major__
	#define NME_COMPILER_VERSION_MINOR __clang_minor__
#elif NME_COMPILER_MSVC
	#define NME_COMPILER_VERSION_MAJOR (_MSC_VER / 100)
	#define NME_COMPILER_VERSION_MINOR (_MSC_VER % 100)
#elif NME_COMPILER_GCC
	#define NME_COMPILER_VERSION_MAJOR __GNUC__
	#define NME_COMPILER_VERSION_MINOR __GNUC_MINOR__
#endif

// Human-readable name (handy for logging and the sandbox banner).
#if NME_COMPILER_CLANG
	#define NME_COMPILER_NAME "Clang"
#elif NME_COMPILER_MSVC
	#define NME_COMPILER_NAME "MSVC"
#elif NME_COMPILER_GCC
	#define NME_COMPILER_NAME "GCC"
#endif

// ============================================================================
// Architecture Detection
// ============================================================================
// NME targets two CPU families across two address sizes. x64 and ARM64 cover
// the desktop tier; x86 and ARM32 cover legacy desktop and embedded.

#if defined(_M_X64) || defined(__x86_64__)
	#define NME_ARCH_X64 1
#else
	#define NME_ARCH_X64 0
#endif

#if defined(_M_IX86) || defined(__i386__)
	#define NME_ARCH_X86 1
#else
	#define NME_ARCH_X86 0
#endif

#if defined(_M_ARM64) || defined(__aarch64__)
	#define NME_ARCH_ARM64 1
#else
	#define NME_ARCH_ARM64 0
#endif

#if defined(_M_ARM) || defined(__arm__)
	#define NME_ARCH_ARM32 1
#else
	#define NME_ARCH_ARM32 0
#endif

#if (NME_ARCH_X64 + NME_ARCH_X86 + NME_ARCH_ARM64 + NME_ARCH_ARM32) != 1
	#error "NME: unsupported architecture"
#endif

#if NME_ARCH_X64
	#define NME_ARCH_NAME "x64"
#elif NME_ARCH_X86
	#define NME_ARCH_NAME "x86"
#elif NME_ARCH_ARM64
	#define NME_ARCH_NAME "ARM64"
#elif NME_ARCH_ARM32
	#define NME_ARCH_NAME "ARM32"
#endif

// ============================================================================
// Pointer Size
// ============================================================================

#if NME_ARCH_X64 || NME_ARCH_ARM64
	#define NME_PTR_SIZE 8
	#define NME_64BIT    1
	#define NME_32BIT    0
#elif NME_ARCH_X86 || NME_ARCH_ARM32
	#define NME_PTR_SIZE 4
	#define NME_64BIT    0
	#define NME_32BIT    1
#endif

#ifndef NME_CACHE_LINE_SIZE
    #if defined(__APPLE__) && defined(__aarch64__)
        #define NME_CACHE_LINE_SIZE 128
    #else
        #define NME_CACHE_LINE_SIZE 64
    #endif
#endif

// ============================================================================
// Endianness
// ============================================================================
// Every architecture NME targets runs little endian on the platforms it cares
// about. Kept in detection form so a future big endian target only needs to
// touch this block.

#if NME_ARCH_X64 || NME_ARCH_X86 || NME_ARCH_ARM64 || NME_ARCH_ARM32
	#define NME_LITTLE_ENDIAN 1
	#define NME_BIG_ENDIAN    0
#else
	#error "NME: unknown endianness for this architecture"
#endif

// ============================================================================
// Build Configuration
// ============================================================================
// NME_DEBUG is 1 unless compiled with NDEBUG. MSVC also defines _DEBUG in debug
// configurations, honored here for consistency with Windows tooling.

#if defined(NDEBUG)
	#define NME_DEBUG 0
#elif defined(_DEBUG)
	#define NME_DEBUG 1
#else
	#define NME_DEBUG 1
#endif

// ============================================================================
// Compiler Attributes
// ============================================================================

#if NME_COMPILER_MSVC
	#define NME_FORCEINLINE __forceinline
	#define NME_FUNCSIG		__FUNCSIG__
#elif NME_COMPILER_CLANG || NME_COMPILER_GCC
	#define NME_FORCEINLINE inline __attribute__((always_inline))
	#define NME_FUNCSIG __PRETTY_FUNCTION__
#else
	#define NME_FORCEINLINE inline
#endif

// ============================================================================
// C++ Standard & Feature Checks
// ============================================================================

#if defined(_MSC_VER)
    #define NME_CPLUSPLUS _MSVC_LANG
#else
    #define NME_CPLUSPLUS __cplusplus
#endif
#if NME_CPLUSPLUS < 201703L
    #error "nme requires C++17 or later"
#endif
#if !defined(__cpp_if_constexpr)
    #error "nme requires 'if constexpr' (C++17)"
#endif
#if !defined(__cpp_fold_expressions)
    #error "nme requires fold expressions (C++17)"
#endif
#if !defined(__cpp_structured_bindings)
    #error "nme requires structured bindings (C++17)"
#endif

// Branch-prediction hints. In C++20 these would be the [[likely]]/[[unlikely]]
// attributes; under C++17 we use the compiler builtins (no-op on MSVC).
#if defined(NME_COMPILER_MSVC)
    #define NME_LIKELY(x)     (x)
    #define NME_UNLIKELY(x)   (x)
#elif defined(NME_COMPILER_CLANG) || defined(NME_COMPILER_GCC)
    #define NME_LIKELY(x) (__builtin_expect(!!(x), 1))
    #define NME_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#endif
