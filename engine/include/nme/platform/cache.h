#pragma once

#include "nme/platform/types.h"

// Architectural L1 line. 64B on x86-64; Apple M-series is 128B; other arm64
// vary -- so it stays a platform knob, never a literal sprinkled at call sites.
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define NME_CACHE_LINE_SIZE 64
#elif defined(__APPLE__) && defined(__aarch64__)
    #define NME_CACHE_LINE_SIZE 128    // Apply Sillicon L1 line is 128
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define NME_CACHE_LINE_SIZE 64
#else
    #define NME_CACHE_LINE_SIZE 64    // safe desktop default
#endif

// Padding stride that actually stops false sharing on x86. The adjacent-line
// prefetcher fetches each 64B line together with its 128B-aligned neighbor, so
// per-thread hot data padded to only 64B can still ping-pong across cores.
// Confirmed on this project's dev box: the stride-test knee sits at ~128B, not
// 64B. Use this (not NME_CACHE_LINE_SIZE) to pad job-system per-worker state.
#if defined(__x86_64__) || defined(_M_X64)
    #define NME_FALSE_SHARING_PAD 128
#else
    #define NME_FALSE_SHARING_PAD NME_CACHE_LINE_SIZE
#endif

namespace nme::platform
{

// Typed mirrors for templates / alignas in constexpr contexts.
inline constexpr u64 kCacheLineSize   = NME_CACHE_LINE_SIZE;
inline constexpr u64 kFalseSharingPad = NME_FALSE_SHARING_PAD; 

}  // namespace nme::platform