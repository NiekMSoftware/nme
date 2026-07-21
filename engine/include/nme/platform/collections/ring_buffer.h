//
// Created by niek on 7/20/2026.
//

#ifndef NME_COLLECTIONS_RING_BUFFER_H_
#define NME_COLLECTIONS_RING_BUFFER_H_

#include "nme/platform/memory/allocator.h"
#include "nme/platform/types.h"

namespace nme {

template <typename T>
struct RingBuffer {
    T*        pData;
    usize     m_count;
    usize     m_capacity;   // power of two
    usize     m_head;       // read cursor
    usize     m_tail;       // write cursor
    Allocator m_alloc;
};

// --- construction ---

template<typename T>
inline void ring_buffer_init(RingBuffer<T>* r, Allocator alloc, const usize capacity) {
    NME_ASSERT(capacity >= 2 && (capacity & (capacity - 1)) == 0);  // pow2
    r->pData = static_cast<T*>(nme::alloc(&alloc, capacity * sizeof(T), alignof(T)));
    NME_VERIFY(r->pData);
    r->m_count = 0;
    r->m_head = 0;
    r->m_tail = 0;
    r->m_capacity = capacity;
    r->m_alloc = alloc;
}

template<typename T>
inline void ring_buffer_destroy(RingBuffer<T>* r) {
    if (!r) return;
    NME_ASSERT(r->m_count == 0);

    if (r->pData) nme::free(&r->m_alloc, r->pData, r->m_capacity * sizeof(T));

    r->pData = nullptr;
    r->m_count = 0;
    r->m_head = 0;
    r->m_tail = 0;
    r->m_capacity = 0;
}

}  // namespace nme

#endif  // NME_COLLECTIONS_RING_BUFFER_H_
