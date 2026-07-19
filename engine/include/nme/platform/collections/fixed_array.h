#ifndef NME_COLLECTIONS_FIXED_ARRAY_H_
#define NME_COLLECTIONS_FIXED_ARRAY_H_

#include <new>  // placement new

#include "nme/platform/debug/assert.h"
#include "nme/platform/types.h"

namespace nme {

// Inline fixed-capacity array with in-struct storage.
// No allocation or allocator; capacity N is compile-time fixed.
// Trivially relocatable as raw storage plus size.
template<typename T, usize N>
struct FixedArray {
    alignas(T) u8 m_storage[sizeof(T) * N];
    usize         m_size;

    // access by element so we could do 'arr[i]', explicit 'fixed_array_at(arr, i)' is also fine
    T& operator[](usize i) noexcept { return m_storage[i]; }
    const T& operator[](usize i) const noexcept { return m_storage[i]; }
};

// Sets size to 0. Does not touch storage. Prefer this (or '={}') over a bare declaration
// so m_size is never garbage.
template<typename T, usize N>
inline void fixed_array_init(FixedArray<T, N>* a) { a->m_size = 0; }

// --- element access ---

template<typename T, usize N>
inline T* fixed_array_data(FixedArray<T, N>* a) {
    return reinterpret_cast<T*>(a->m_storage);
}
template<typename T, usize N>
inline const T* fixed_array_data(const FixedArray<T, N>* a) {
    return reinterpret_cast<const T*>(a->m_storage);
}

template<typename T, usize N>
inline T& fixed_array_at(FixedArray<T, N>* a, const usize i) {
    NME_PLATFORM_ASSERT(i < a->m_size);
    return fixed_array_data(a)[i];
}
template<typename T, usize N>
inline const T& fixed_array_at(const FixedArray<T, N>* a, const usize i) {
    NME_PLATFORM_ASSERT(i < a->m_size);
    return fixed_array_data(a)[i];
}

// --- queries ---

template<typename T, usize N>
inline usize fixed_array_size(const FixedArray<T, N>* a)      { return a->m_size; }
template<typename T, usize N>
constexpr usize fixed_array_capacity(const FixedArray<T, N>*) { return N; }
template<typename T, usize N>
constexpr bool fixed_array_empty(const FixedArray<T, N>* a)   { return a->m_size == 0; }
template<typename T, usize N>
constexpr bool fixed_array_full(const FixedArray<T, N>* a)    { return a->m_size == N; }

// --- mutation ---

// Construct a copy at the end, returns the new slot or nullptr if full.
template<typename T, usize N>
inline T* fixed_array_push(FixedArray<T, N>* a, const T& value) {
    if (a->m_size == N) return nullptr;
    T* slot = fixed_array_data(a) + a->m_size;
    ::new (static_cast<void*>(slot)) T(value);
    ++a->m_size;
    return slot;
}

// Destroy the last element.
template<typename T, usize N>
inline void fixed_array_pop(FixedArray<T, N>* a) {
    NME_PLATFORM_ASSERT(a->m_size > 0);
    --a->m_size;
    fixed_array_data(a)[a->m_size].~T();
}

// O(1) unordered erase, overwrite the hole with the last element, then pop.
template<typename T, usize N>
inline void fixed_array_erase_swap(FixedArray<T, N>* a, const usize i) {
    NME_PLATFORM_ASSERT(i < a->m_size);
    T* data = fixed_array_data(a);
    const usize last = a->m_size - 1;
    if (i != last) {
        data[i] = static_cast<T&&>(data[last]);     // move-assign (copy fallback)
    }
    data[last].~T();
    a->m_size = last;
}

// Destroy all elements, capacity is unchanged
template<typename T, usize N>
inline void fixed_array_clear(FixedArray<T, N>* a) {
    T* data = fixed_array_data(a);
    for (usize i = 0; i < a->m_size; ++i) data[i].~T();
    a->m_size = 0;
}

}  // namespace nme

#endif  // NME_COLLECTIONS_FIXED_ARRAY_H_
