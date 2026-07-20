#ifndef NME_SCRATCH_ALLOC_H_
#define NME_SCRATCH_ALLOC_H_

#include "nme/platform/memory/allocator.h"
#include "nme/platform/memory/align.h"
#include "nme/core/memory/stack_alloc.h"

namespace nme {

struct ScratchAllocator {
    StackAllocator m_stack[2];
    u32            m_current;
};

// --- scratch alloc ---

inline void scratch_alloc_init(ScratchAllocator* s, void* pBackingA, void* pBackingB, const usize bytes) {
    stack_alloc_init(&s->m_stack[0], pBackingA, bytes);
    stack_alloc_init(&s->m_stack[1], pBackingB, bytes);
    s->m_current = 0;
}
inline void scratch_alloc_swap(ScratchAllocator* s) {
    s->m_current = static_cast<u32>(!s->m_current);
    stack_alloc_clear(&s->m_stack[s->m_current]);
}
inline void* scratch_alloc(ScratchAllocator* s, const usize bytes, const usize align) {
    return stack_alloc(&s->m_stack[s->m_current], bytes, align);
}

// --- interface adapter ---

inline void* scratch_alloc_vtbl(void* pSelf, const usize bytes, const usize align) {
    return scratch_alloc(static_cast<ScratchAllocator*>(pSelf), bytes, align);
}
inline void scratch_free_vtbl(void* /*self*/, void* /*p*/, usize /*bytes*/) {}
inline Allocator scratch_as_allocator(ScratchAllocator* s) {
    Allocator out{};
    out.alloc = scratch_alloc_vtbl;
    out.free  = scratch_free_vtbl;
    out.self  = s;
    return out;
}

}  // namespace nme

#endif  // NME_SCRATCH_ALLOC_H_
