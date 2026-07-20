#pragma once

#include <new>          // placement new
#include <utility>      // std::forward

#include "nme/core/memory/allocator.h"
#include "nme/core/result/error.h"
#include "nme/platform/collections/dynamic_array.h"
#include "nme/platform/debug/assert.h"
#include "nme/platform/types.h"

namespace nme {

class Subsystem;

class Kernel {
    DynamicArray<Subsystem*> m_subsystems{}; // owned; destroyed in reverse on shutdown
    usize                    m_started = 0;  // count currently up (a prefix of m_subsystems)
    Allocator                m_alloc;        // where subsystems + the array are allocated

public:
    explicit Kernel(const Allocator& alloc) : m_alloc(alloc) {
        dynamic_array_init(&m_subsystems, alloc);
    }
    ~Kernel() { shutdown(); }

    Kernel(const Kernel&)            = delete;
    Kernel& operator=(const Kernel&) = delete;

    template<class T, class... Args>
    T* add(Args&&... args);

    [[nodiscard]] Error startup();

    void shutdown();
};

template <class T, class... Args>
T* Kernel::add(Args&&... args) {
    NME_ASSERT(m_started == 0);
    void* mem = alloc(&m_alloc, sizeof(T), alignof(T));
    NME_VERIFY(mem);
    T* sys = ::new (mem) T(std::forward<Args>(args)...);

    Subsystem* base = sys;
    dynamic_array_push(&m_subsystems, base);
    return sys;
}

}  // namespace nme