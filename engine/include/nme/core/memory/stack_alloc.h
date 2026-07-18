#ifndef NME_STACK_ALLOC_H_
#define NME_STACK_ALLOC_H_

#include "nme/core/memory/align.h"
#include "nme/core/memory/allocator.h"

namespace nme {

struct StackAllocator {
    u8*   pBase;        // start of the backing block
    usize m_capacity;   // total bytes
    usize m_top;        // bytes in use == offset of next free bytes
};
typedef usize StackMarker;

// --- stack allocator ---

inline void stack_alloc_init(StackAllocator* a, void* backing, const usize bytes) {
    a->pBase = static_cast<u8*>(backing);
    a->m_capacity = bytes;
    a->m_top = 0;
}

inline void* stack_alloc(StackAllocator* a, const usize bytes, const usize align) {
    const uptr curr = reinterpret_cast<uptr>(a->pBase) + a->m_top;
    const uptr aligned = align_up(curr, align);
    const usize used   = static_cast<usize>(aligned - reinterpret_cast<uptr>(a->pBase)) + bytes;
    if (used > a->m_capacity) return nullptr;   // out of space
    a->m_top = used;
    return reinterpret_cast<void*>(aligned);
}

inline StackMarker stack_alloc_get_marker(const StackAllocator* a) { return a->m_top; }
inline void stack_alloc_free_to_marker(StackAllocator* a, const StackMarker m) {
    NME_ASSERT(m <= a->m_top);
    a->m_top = m;
}
inline void stack_alloc_clear(StackAllocator* a) { a->m_top = 0; }

// --- interface adapter ---

inline void* stack_alloc_vtbl(void* self, const usize bytes, const usize align) {
    return stack_alloc(static_cast<StackAllocator*>(self), bytes, align);
}
inline void stack_alloc_free_vtbl(void* /*self*/, void* /*p*/, usize /*bytes*/) {}
inline Allocator stack_as_allocator(StackAllocator* a) {
    Allocator out{};
    out.alloc = stack_alloc_vtbl;
    out.free  = stack_alloc_free_vtbl;
    out.self  = a;
    return out;
}

}  // namespace nme

#endif  // NME_STACK_ALLOC_H_
