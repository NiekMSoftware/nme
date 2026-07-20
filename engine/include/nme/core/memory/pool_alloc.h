#ifndef NME_POOL_ALLOC_H_
#define NME_POOL_ALLOC_H_

#include "nme/platform/memory/allocator.h"
#include "nme/platform/memory/align.h"

namespace nme {

struct PoolAllocator {
    void* pFreeList;        // head of singly-linked free list
    u8*   pBase;
    usize m_blockSize;      // aligned free
    usize m_blockCount;
    usize m_align;
};

// --- pool alloc ---

inline void pool_alloc_init(PoolAllocator* p, void* pBacking,
                            usize block_size, const usize block_count, const usize align) {
    if (block_size < sizeof(void*)) block_size = sizeof(void*);
    block_size = static_cast<usize>(align_up(block_size, align));

    NME_ASSERT((reinterpret_cast<uptr>(pBacking) & (align - 1)) == 0); // backing must be aligned
    p->pBase        = static_cast<u8*>(pBacking);
    p->m_blockSize  = block_size;
    p->m_blockCount = block_count;
    p->m_align        = align;
    p->pFreeList    = nullptr;

    // thread list, low block on top
    for (usize i = block_count; i-- > 0;) {
        void* block = p->pBase + i * block_size;
        *static_cast<void**>(block) = p->pFreeList;
        p->pFreeList = block;
    }
}

inline void* pool_alloc(PoolAllocator* p) {
    if (!p->pFreeList) return nullptr;
    void* block = p->pFreeList;
    p->pFreeList = *static_cast<void**>(block);     // pop
    return block;
}
inline void pool_free(PoolAllocator* p, void* pBlock) {
    if (!pBlock) return;
    *static_cast<void**>(pBlock) = p->pFreeList;    // push
    p->pFreeList = pBlock;
}

// --- interface adapter ---

inline void* pool_alloc_vtbl(void* pSelf, const usize bytes, const usize align) {
    auto* p = static_cast<PoolAllocator*>(pSelf);
    NME_ASSERT(bytes <= p->m_blockSize && align <= p->m_align);
    (void)bytes; (void)align;
    return pool_alloc(p);
}
inline void pool_free_vtbl(void* pSelf, void* p, usize /*bytes*/) {
    pool_free(static_cast<PoolAllocator*>(pSelf), p);
}
inline Allocator pool_as_allocator(PoolAllocator* p) {
    Allocator out{};
    out.alloc = pool_alloc_vtbl;
    out.free  = pool_free_vtbl;
    out.self  = p;
    return out;
}

}  // namespace nme

#endif  // NME_POOL_ALLOC_H_
