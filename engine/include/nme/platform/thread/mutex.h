#pragma once

#include "nme/platform/types.h"

namespace nme
{

class Mutex {
    static constexpr usize kNativeSize  = 64;
    static constexpr usize kNativeAlign = 16;
    alignas(kNativeAlign) u8 m_storage[kNativeSize]{};

public:
    Mutex();
    ~Mutex();

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    void lock();
    bool tryLock();
    void unlock();

    [[nodiscard]] void* native() noexcept;
};

}  // namespace nme
