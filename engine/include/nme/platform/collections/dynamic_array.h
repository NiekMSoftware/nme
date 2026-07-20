#ifndef NME_COLLECTIONS_DYNAMIC_ARRAY_H_
#define NME_COLLECTIONS_DYNAMIC_ARRAY_H_

#include "nme/core/memory/allocator.h"
#include "nme/platform/types.h"

namespace nme {

template<typename T>
struct DynamicArray {
    T*        pData;        // heap block, or nullptr when empty
    usize     m_size;       // number of constructed elements
    usize     m_capacity;   // number of slots the block can hold
    Allocator m_alloc;      // where growth + free comes from

    T& operator[](usize i)             { return reinterpret_cast<T*>(pData)[i]; }
    const T& operator[](usize i) const { return reinterpret_cast<T*>(pData)[i]; }
};

template<typename T>
inline void dynamic_array_init(DynamicArray<T>* a) {
    a->pData = nullptr;
    a->m_size = 0;
    a->m_capacity = 0;
}

template<typename T>
inline void dynamic_array_destroy(DynamicArray<T>* a) {
    for (usize i = 0; i < a->m_size; ++i) a->pData[i].~T();
    if (a->pData) free(&a->m_alloc, a->pData, a->m_capacity * sizeof(T));
    a->pData = nullptr;
    a->m_size = 0;
    a->m_capacity = 0;
}

}  // namespace nme

#endif  // NME_COLLECTIONS_DYNAMIC_ARRAY_H_
