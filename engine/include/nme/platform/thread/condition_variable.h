#pragma once

#include "nme/platform/types.h"

namespace nme {

class Mutex;

// A condition variable, paired with nme::Mutex.
class ConditionVariable {
    static constexpr usize kNativeSize  = 64;
    static constexpr usize kNativeAlign = 16;
    alignas(kNativeAlign) u8 m_storage[kNativeSize]{};

public:
    ConditionVariable();
    ~ConditionVariable();

    ConditionVariable(const ConditionVariable&)            = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;
    ConditionVariable(ConditionVariable&&)                 = delete;
    ConditionVariable& operator=(ConditionVariable&&)      = delete;

    // `mutex` must be called by the caller. wait() automatically releases it,
    // sleeps until notified, and re-acquires before returning.
    void wait(Mutex& mutex);

    void notifyOne() noexcept;  // wake one waiter (a job arrived)
    void notifyAll() noexcept;  // wake everyone (shutdown)
};

}  // namespace nme
