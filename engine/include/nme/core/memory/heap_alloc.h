#ifndef NME_MEMORY_HEAP_ALLOC_H_
#define NME_MEMORY_HEAP_ALLOC_H_

#include "nme/platform/thread/atomics.h"
#include "nme/platform/types.h"

namespace nme {

enum class MemoryTag : u32 {
    kDefault,
    kRenderer,
    kPhysics,
    kAnimation,
    kResources,
    kFiberStacks,       // TODO: pool backing for the job system's fiber stacks
    kJobs,
    kCount
};
constexpr usize kMemTagCount = static_cast<usize>(MemoryTag::kCount);

// Packed before the user pointer, carries everything free() needs.
struct HeapHeader {
    void*     base;
    usize     size;
    MemoryTag tag;
};

// General purpose heap, the root backing source + the variable-size fallback.
// Per-tag live + peak bytes for the debug overlay.
struct HeapAllocator {
    Atomic<iptr> m_used[kMemTagCount];  // live bytes, per tag
    Atomic<iptr> m_peak[kMemTagCount];  // high water, per tag
};

namespace detail {

// Lock-free atomic max. Peak only grows, so this is all the sync it needs.
inline void heap_peak_max(Atomic<iptr>& peak, const iptr value) noexcept {
    iptr prev = peak.load(MemoryOrder::Relaxed);
    while (value > prev &&
           !peak.compareExchangeWeak(prev, value, MemoryOrder::Relaxed)) {
        // compareExchangeWeak refreshes 'prev' on failure -> re-test the guard
    }
}

}  // namespace detail



}  // namespace nme

#endif  // NME_MEMORY_HEAP_ALLOC_H_
