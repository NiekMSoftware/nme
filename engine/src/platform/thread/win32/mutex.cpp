#include "nme/platform/thread/mutex.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <new>  // placement new, std::launder

namespace nme
{

static SRWLOCK *native(u8 *s) {
    return std::launder(reinterpret_cast<SRWLOCK*>(s));
}

Mutex::Mutex() {
    static_assert(sizeof(SRWLOCK)  <= kNativeSize,  "grow Mutex::kNativeSize");
    static_assert(alignof(SRWLOCK) <= kNativeAlign, "grow Mutex::kNativeAlign");
    InitializeSRWLock(native(m_storage));
}

Mutex::~Mutex() { /* SRWLock requires no teardown */ }

void Mutex::lock() { AcquireSRWLockExclusive(native(m_storage)); }
bool Mutex::tryLock() { return TryAcquireSRWLockExclusive(native(m_storage)) != 0; }
void Mutex::unlock() { ReleaseSRWLockExclusive(native(m_storage)); }

}  // namespace nme