#ifndef NME_MEMORY_HEAP_ALLOC_H_
#define NME_MEMORY_HEAP_ALLOC_H_

#include "align.h"
#include "allocator.h"
#include "nme/core/assert/assert.h"
#include "nme/platform/thread/atomics.h"
#include "nme/platform/types.h"

namespace nme {

enum class MemTag : u32 {
    kDefault,
    kRenderer,
    kPhysics,
    kAnimation,
    kResources,
    kFiberStacks,       // TODO: pool backing for the job system's fiber stacks
    kJobs,
    kCount
};
constexpr usize kMemTagCount = static_cast<usize>(MemTag::kCount);

// Packed before the user pointer, carries everything free() needs.
struct HeapHeader {
    void*     base;
    usize     size;
    MemTag    tag;
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

// --- heap alloc ---

inline void heap_alloc_init(HeapAllocator* h) {
    for (usize i = 0; i < kMemTagCount; ++i) {
        h->m_used[i].store(0, MemoryOrder::Relaxed);
        h->m_peak[i].store(0, MemoryOrder::Relaxed);
    }
}

inline void* heap_alloc_tagged(HeapAllocator* h, const usize bytes, usize align, const MemTag tag) {
    NME_ASSERT(align != 0 && (align & (align - 1)) == 0);   // power of two
    if (align < alignof(HeapHeader)) align = alignof(HeapHeader);

    // one block: [ pad | HeapHeader | user bytes ], user aligned to 'align'
    void* base = std::malloc(bytes + align + sizeof(HeapHeader));
    if (!base) return nullptr;

    const uptr raw = reinterpret_cast<uptr>(base) + sizeof(HeapHeader);
    const uptr user = align_up(raw, align);
    HeapHeader* hdr = reinterpret_cast<HeapHeader*>(user) - 1;
    hdr->base = base;
    hdr->size = bytes;
    hdr->tag  = tag;

    const auto i = static_cast<usize>(tag);
    const iptr now = h->m_used[i].fetchAdd(static_cast<iptr>(bytes), MemoryOrder::Relaxed)
                     + static_cast<iptr>(bytes);            // fetchAdd returns prior value
    detail::heap_peak_max(h->m_peak[i], now);
    return reinterpret_cast<void*>(user);
}

inline void heap_free(HeapAllocator* h, void* p) noexcept {
    if (!p) return;
    const HeapHeader* hdr = static_cast<HeapHeader*>(p) - 1;
    h->m_used[static_cast<usize>(hdr->tag)]
        .fetchSub(static_cast<iptr>(hdr->size), MemoryOrder::Relaxed);
    std::free(hdr->base);       // tag ignores on free, header is authority
}

inline void* heap_alloc(HeapAllocator* h, const usize bytes, const usize align) {
    return heap_alloc_tagged(h, bytes, align, MemTag::kDefault);
}

// --- stats for memory overlay ---

inline iptr heap_bytes_used(const HeapAllocator* h, const MemTag tag) {
    return h->m_used[static_cast<usize>(tag)].load(MemoryOrder::Relaxed);
}
inline iptr heap_bytes_peak(const HeapAllocator* h, const MemTag tag) {
    return h->m_peak[static_cast<usize>(tag)].load(MemoryOrder::Relaxed);
}

// --- interface adapter ---

inline void* heap_alloc_vtbl(void* pSelf, const usize bytes, const usize align) {
    return heap_alloc(static_cast<HeapAllocator*>(pSelf), bytes, align);
}
inline void heap_free_vtbl(void* pSelf, void* p, usize /*bytes*/) {
    heap_free(static_cast<HeapAllocator*>(pSelf), p);
}
inline Allocator heap_as_allocator(HeapAllocator* h) {
    Allocator out{};
    out.alloc = heap_alloc_vtbl;
    out.free  = heap_free_vtbl;
    out.self  = h;
    return out;
}

// --- tagged view ---
// the tag rides on the handle, so it's fiber-migration-safe

struct TaggedHeap {
    HeapAllocator* pHeap;
    MemTag         m_tag;
};

inline void* tagged_heap_alloc_vtbl(void* pSelf, const usize bytes, const usize align) {
    const auto* t = static_cast<TaggedHeap*>(pSelf);
    return heap_alloc_tagged(t->pHeap, bytes, align, t->m_tag);
}
inline void tagged_heap_free_vtbl(void* pSelf, void* p, usize /*bytes*/) {
    heap_free(static_cast<TaggedHeap*>(pSelf)->pHeap, p);   // header carries the real tag
}
inline Allocator tagged_heap_as_allocator(TaggedHeap* t) {
    Allocator out{};
    out.alloc = tagged_heap_alloc_vtbl;
    out.free  = tagged_heap_free_vtbl;
    out.self  = t;
    return out;
}

}  // namespace nme

#endif  // NME_MEMORY_HEAP_ALLOC_H_
