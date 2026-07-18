#ifndef NME_POOL_ALLOC_H_
#define NME_POOL_ALLOC_H_

#include "nme/core/memory/align.h"
#include "nme/core/memory/allocator.h"

namespace nme {

struct PoolAllocator {
    void* pFreeList;        // head of singly-linked free list
    u8*   pBase;
    usize m_blockSize;      // aligned free
    usize m_blockCount;
    usize align;
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
    p->align        = align;
    p->pFreeList    = nullptr;

    // thread list, low block on top
    for (usize i = block_count; i-- > 0;) {
        void* block = p->pBase + i * block_size;
        *static_cast<void**>(block) = p->pFreeList;
        p->pFreeList = block;
    }
}

inline void* pool_alloc(PoolAllocator* p, usize bytes) {
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

}  // namespace nme

#endif  // NME_POOL_ALLOC_H_
