#ifndef NME_COLLECTIONS_HASH_MAP_H_
#define NME_COLLECTIONS_HASH_MAP_H_

#include <new>  // placement new

#include "nme/core/string/string_id.h"
#include "nme/platform/memory/allocator.h"
#include "nme/platform/types.h"

namespace nme {

template<typename V>
struct HashMap {
    StringId* pKeys;
    V*        pValues;
    usize     m_count;
    usize     m_capacity;  // pow2
    usize     m_mask;
    Allocator m_alloc;
};

namespace detail {
inline bool hash_map_overloaded(const usize count, const usize capacity) {
    return count * 10 >= capacity * 7;
}
}  // namespace detail

// --- lifetime ---

template<typename V>
inline void hash_map_init(HashMap<V>* m, Allocator a, usize capacity) {
    if (capacity < 8) capacity = 8;
    NME_ASSERT((capacity & (capacity - 1)) == 0);

    m->pKeys = static_cast<StringId*>(alloc(&a, capacity * sizeof(StringId), alignof(StringId)));
    m->pValues = static_cast<V*>(alloc(&a, capacity * sizeof(V), alignof(V)));

    for (usize i = 0; i < capacity; ++i) m->pKeys[i].value = 0; // all empty
    m->m_count    = 0;
    m->m_capacity = capacity;
    m->m_mask     = capacity - 1;
    m->m_alloc    = a;
}

template<typename V>
inline void hash_map_destroy(HashMap<V>* m) {
    if (!m) return;
    for (usize i = 0; i < m->m_capacity; ++i) {
        if (m->pKeys[i].value != 0)
            m->pValues[i].~V();     // destroy live elements
    }
    if (m->pKeys)   free(&m->m_alloc, m->pKeys,   m->m_capacity * sizeof(StringId));
    if (m->pValues) free(&m->m_alloc, m->pValues, m->m_capacity * sizeof(V));
    m->pKeys = nullptr;
    m->pValues = nullptr;
    m->m_count = m->m_capacity = m->m_mask = 0;
}

template<typename V>
inline usize hash_map_count(const HashMap<V>* m) { return m->m_count; }
template<typename V>
inline bool hash_map_empty(const HashMap<V>* m) { return m->m_count == 0; }

// --- lookup ---

template<typename V>
inline V* hash_map_find(const HashMap<V>* m, const StringId id) {
    NME_ASSERT(id.value != 0);
    usize slot = id.value & m->m_mask;
    while (m->pKeys[slot].value != 0) {
        if (m->pKeys[slot].value == id.value) return &m->pValues[slot];
        slot = (slot + 1) & m->m_mask;
    }
    return nullptr;
}
template<typename V>
inline const V* hash_map_find(const HashMap<V>* m, const StringId id) {
    NME_ASSERT(id.value != 0);
    usize slot = id.value & m->m_mask;
    while (m->pKeys[slot].value != 0) {
        if (m->pKeys[slot].value == id.value) return &m->pValues[slot];
        slot = (slot + 1) & m->m_mask;
    }
    return nullptr;
}

template<typename V>
inline bool hash_map_contains(const HashMap<V>* m, const StringId id) {
    return hash_map_find(m, id) != nullptr;
}

}  // namespace nme

#endif  // NME_COLLECTIONS_HASH_MAP_H_
