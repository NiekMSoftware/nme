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
inline V* hash_map_find(HashMap<V>* m, const StringId id) {
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

// --- grow / rehash ---

// Reinsert every live entry into a larger set of arrays. Positions depend on
// the mask, which changes, so this re-probes rather than copying slot-for-slot.
template<typename V>
inline void hash_map_rehash(HashMap<V>* m, const usize new_capacity) {
    NME_ASSERT((new_capacity & (new_capacity - 1)) == 0 && new_capacity >= m->m_capacity);

    StringId*   old_keys   = m->pKeys;
    V*          old_values = m->pValues;
    const usize old_cap  = m->m_capacity;

    m->pKeys   = static_cast<StringId*>(alloc(&m->m_alloc, new_capacity * sizeof(StringId), alignof(StringId)));
    m->pValues = static_cast<V*>(alloc(&m->m_alloc, new_capacity * sizeof(V), alignof(V)));
    NME_VERIFY(m->pKeys && m->pValues);
    for (usize i = 0; i < new_capacity; ++i) m->pKeys[i].value = 0;
    m->m_capacity = new_capacity;
    m->m_mask     = new_capacity - 1;

    // move each live entry into its new home, destroying the old slot
    for (usize i = 0; i < old_cap; ++i) {
        if (old_keys[i].value == 0) continue;
        usize slot = old_keys[i].value & m->m_mask;
        while (m->pKeys[slot].value != 0) slot = (slot + 1) & m->m_mask;   // find empty
        m->pKeys[slot] = old_keys[i];
        ::new (static_cast<void*>(m->pValues + slot)) V(static_cast<V&&>(old_values[i]));  // move-construct
        old_values[i].~V();                                                                // destroy old
    }

    free(&m->m_alloc, old_keys,   old_cap * sizeof(StringId));
    free(&m->m_alloc, old_values, old_cap * sizeof(V));
}

// --- insert / remove ---

template<typename V>
inline V* hash_map_insert(HashMap<V>* m, const StringId id, const V& value) {
    NME_ASSERT(id.value != 0);

    usize slot = id.value & m->m_mask;
    while (m->pKeys[slot].value != 0) {
        if (m->pKeys[slot].value == id.value) {
            m->pValues[slot] = value;
            return &m->pValues[slot];
        }
        slot = (slot + 1) & m->m_mask;
    }

    if (detail::hash_map_overloaded(m->m_count + 1, m->m_capacity)) {
        hash_map_rehash(m, m->m_capacity * 2);
        slot = id.value & m->m_mask;
        while (m->pKeys[slot].value != 0) slot = (slot + 1) & m->m_mask;
    }

    m->pKeys[slot] = id;
    ::new (static_cast<void*>(m->pValues + slot)) V(value);     // construct in place
    ++m->m_count;
    return &m->pValues[slot];
}

template<typename V>
inline bool hash_map_remove(HashMap<V>* m, const StringId id) {
    NME_ASSERT(id.value != 0);

    usize slot = id.value & m->m_mask;
    while (m->pKeys[slot].value != 0) {
        if (m->pKeys[slot].value == id.value)
            break;
        slot = (slot + 1) & m->m_mask;
    }
    if (m->pKeys[slot].value == 0) return false;    // not found

    m->pValues[slot].~V();  // destroy the removed value

    usize hole = slot;
    usize next = (slot + 1) & m->m_mask;
    while (m->pKeys[next].value != 0) {
        const usize ideal = m->pKeys[next].value & m->m_mask;
        if (((next - ideal) & m->m_mask) >= ((next - hole) & m->m_mask)) {
            m->pKeys[hole] = m->pKeys[next];
            m->pValues[hole] = static_cast<V&&>(m->pValues[next]);
            m->pValues[next].~V();
            hole = next;
        }
        next = (next + 1) & m->m_mask;
    }
    m->pKeys[hole].value = 0;
    --m->m_count;
    return true;
}

template<typename V>
inline void hash_map_clear(HashMap<V>* m) {
    for (usize i = 0; i < m->m_capacity; ++i) {
        if (m->pKeys[i].value != 0) {
            m->pValues[i].~V();
            m->pKeys[i].value = 0;
        }
    }
    m->m_count = 0;
}

}  // namespace nme

#endif  // NME_COLLECTIONS_HASH_MAP_H_
