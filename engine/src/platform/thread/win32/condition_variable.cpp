#include "nme/platform/thread/condition_variable.h"
#include "nme/platform/thread/mutex.h"
#include "nme/platform/platform.h"

#include <new>  // placement new

#if NME_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif
#include <windows.h>

namespace nme {
namespace {
inline CONDITION_VARIABLE* cv(u8* s) { return reinterpret_cast<CONDITION_VARIABLE*>(s); }
inline const CONDITION_VARIABLE* cv(const u8* s) { return reinterpret_cast<const CONDITION_VARIABLE*>(s); }
}

ConditionVariable::ConditionVariable() {
    static_assert(sizeof(CONDITION_VARIABLE) <= kNativeSize, "grow ConditionVariable::kNativeSize");
    static_assert(alignof(CONDITION_VARIABLE) <= kNativeAlign, "grow ConditionVariable::kNativeAlign");
    InitializeConditionVariable(cv(m_storage));
}

ConditionVariable::~ConditionVariable() = default;

void ConditionVariable::wait(Mutex& mutex) {
    SleepConditionVariableSRW(cv(m_storage),
        static_cast<SRWLOCK*>(mutex.native()),
        INFINITE,
        0 /* exclusive: matched Mutex::lock */);
}

void ConditionVariable::notifyOne() noexcept { WakeConditionVariable(cv(m_storage)); }
void ConditionVariable::notifyAll() noexcept { WakeAllConditionVariable(cv(m_storage)); }

}  // namespace nme
