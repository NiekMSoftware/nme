#ifndef NME_COLLECTIONS_DYNAMIC_ARRAY_H_
#define NME_COLLECTIONS_DYNAMIC_ARRAY_H_

#include <type_traits>

#include "nme/core/memory/allocator.h"
#include "nme/platform/debug/assert.h"
#include "nme/platform/types.h"

namespace nme {

template<typename T>
struct DynamicArray {
    T*        pData;        // heap block, or nullptr when empty
    usize     m_size;       // number of constructed elements
    usize     m_capacity;   // number of slots the block can hold
    Allocator m_alloc;      // where growth + free comes from

    T& operator[](usize i)             { return pData[i]; }
    const T& operator[](usize i) const { return pData[i]; }
};

template<typename T>
inline void dynamic_array_init(DynamicArray<T>* a, Allocator alloc) {
    a->pData = nullptr;
    a->m_size = 0;
    a->m_capacity = 0;
    a->m_alloc = alloc;
}

template<typename T>
inline void dynamic_array_destroy(DynamicArray<T>* a) {
    for (usize i = 0; i < a->m_size; ++i) a->pData[i].~T();
    if (a->pData) free(&a->m_alloc, a->pData, a->m_capacity * sizeof(T));
    a->pData = nullptr;
    a->m_size = 0;
    a->m_capacity = 0;
}

// --- element access ---

template<typename T>
inline T* dynamic_array_data(DynamicArray<T>* a) {
    return a->pData;
}
template<typename T>
inline const T* dynamic_array_data(const DynamicArray<T>* a) {
    return a->pData;
}

template<typename T>
inline T& dynamic_array_at(DynamicArray<T>* a, const usize i) {
    NME_ASSERT(i < a->m_size);
    return dynamic_array_data(a)[i];
}
template<typename T>
inline const T& dynamic_array_at(const DynamicArray<T>* a, const usize i) {
    NME_ASSERT(i < a->m_size);
    return dynamic_array_data(a)[i];
}

// --- queries ---

template<typename T>
inline usize dynamic_array_size(const DynamicArray<T>* a)     { return a->m_size; }
template<typename T>
inline usize dynamic_array_capacity(const DynamicArray<T>* a) { return a->m_capacity; }
template<typename T>
inline bool dynamic_array_empty(const DynamicArray<T>* a)     { return a->m_size == 0; }

// --- mutation ---

template<typename T>
inline void dynamic_array_reserve(DynamicArray<T>* a, usize new_cap) {
    if (new_cap <= a->m_capacity) return;

    T* pNew = static_cast<T*>(
        alloc(&a->m_alloc, new_cap * sizeof(T), alignof(T)));
    NME_VERIFY(pNew);

    // reallocate old -> new
    if constexpr (std::is_trivially_copyable_v<T>) {
        memcpy(pNew, a->pData, a->m_size * sizeof(T));
    } else {
        for (usize i = 0; i < a->m_size; ++i) {
            ::new (static_cast<void*>(pNew + i)) T(static_cast<T&&>(a->pData[i]));  // move
            a->pData[i].~T();
        }
    }

    if (a->pData) {
        free(&a->m_alloc, a->pData, a->m_capacity * sizeof(T));  // give back capacity
    }
    a->pData      = pNew;
    a->m_capacity = new_cap;
}

template<typename T>
inline T* dynamic_array_push(DynamicArray<T>* a, const T& value) {
    if (a->m_size == a->m_capacity) {
        usize next = a->m_capacity ? a->m_capacity * 2 : 8;
        dynamic_array_reserve(a, next);
    }
    T* slot = a->pData + a->m_size;
    ::new (static_cast<void*>(slot)) T(value);
    ++a->m_size;
    return slot;
}

}  // namespace nme

#endif  // NME_COLLECTIONS_DYNAMIC_ARRAY_H_
