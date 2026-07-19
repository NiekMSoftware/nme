#ifndef NME_COLLECTIONS_FIXED_ARRAY_H_
#define NME_COLLECTIONS_FIXED_ARRAY_H_

#include "nme/platform/debug/assert.h"
#include "nme/platform/types.h"

namespace nme {

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

}  // namespace nme

#endif  // NME_COLLECTIONS_FIXED_ARRAY_H_
