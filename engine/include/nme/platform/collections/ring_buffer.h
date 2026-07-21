//
// Created by niek on 7/20/2026.
//

#ifndef NME_COLLECTIONS_RING_BUFFER_H_
#define NME_COLLECTIONS_RING_BUFFER_H_

#include "nme/platform/memory/allocator.h"
#include "nme/platform/types.h"

#include <new>  // placement new

namespace nme {

template <typename T>
struct RingBuffer {
    T*        pData;
    usize     m_count;
    usize     m_capacity;   // power of two
    usize     m_head;       // read cursor
    usize     m_tail;       // write cursor
    usize     m_mask;
    Allocator m_alloc;
};

// --- mutation ---

template<typename T>
inline bool ring_buffer_push(RingBuffer<T>* r, const T& value) {
    if (r->m_count == r->m_capacity) return false;
    ::new (static_cast<void*>(r->pData + r->m_tail)) T(value);
    r->m_tail = (r->m_tail + 1) & r->m_mask;
    ++r->m_count;
    return true;
}

template<typename T>
inline bool ring_buffer_pop(RingBuffer<T>* r, T* out) {
    if (r->m_count == 0) return false;
    T* slot = r->pData + r->m_head;
    if (out) *out = static_cast<T&&>(*slot);
    slot->~T();
    r->m_head = (r->m_head + 1) & r->m_mask;
    --r->m_count;
    return true;
}

template<typename T>
inline void ring_buffer_clear(RingBuffer<T>* r) {
    usize i = r->m_head;
    for (usize n = 0; n < r->m_count; ++n) {
        r->pData[i].~T();
        i = (i + 1) & r->m_mask;
    }
    r->m_head = r->m_tail = r->m_count = 0;
}

template<typename T>
inline T* ring_buffer_peek(RingBuffer<T>* r) {
    return r->m_count ? r->pData + r->m_head : nullptr;
}
template<typename T>
inline const T* ring_buffer_peek(const RingBuffer<T>* r) {
    return r->m_count ? r->pData + r->m_head : nullptr;
}

// --- construction ---

template<typename T>
inline void ring_buffer_init(RingBuffer<T>* r, Allocator alloc, const usize capacity) {
    NME_ASSERT(capacity >= 2 && (capacity & (capacity - 1)) == 0);  // pow2
    r->pData = static_cast<T*>(nme::alloc(&alloc, capacity * sizeof(T), alignof(T)));
    NME_VERIFY(r->pData);
    r->m_count = 0;
    r->m_head = 0;
    r->m_tail = 0;
    r->m_mask = capacity - 1;
    r->m_capacity = capacity;
    r->m_alloc = alloc;
}

template<typename T>
inline void ring_buffer_destroy(RingBuffer<T>* r) {
    if (!r) return;
    nme::ring_buffer_clear(r);

    if (r->pData) nme::free(&r->m_alloc, r->pData, r->m_capacity * sizeof(T));

    r->pData = nullptr;
    r->m_count = 0;
    r->m_head = 0;
    r->m_tail = 0;
    r->m_capacity = 0;
}

}  // namespace nme

#endif  // NME_COLLECTIONS_RING_BUFFER_H_
