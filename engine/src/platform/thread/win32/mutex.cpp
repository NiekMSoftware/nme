#include "nme/platform/thread/mutex.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <new>  // placement new, std::launder

namespace nme
{

static SRWLOCK *asSwr(u8 *s) {
    return std::launder(reinterpret_cast<SRWLOCK*>(s));
}


Mutex::Mutex() {
    static_assert(sizeof(SRWLOCK)  <= kNativeSize,  "grow Mutex::kNativeSize");
    static_assert(alignof(SRWLOCK) <= kNativeAlign, "grow Mutex::kNativeAlign");
    InitializeSRWLock(asSwr(m_storage));
}

/* SRWLock requires no teardown */
Mutex::~Mutex() = default;

void Mutex::lock() { AcquireSRWLockExclusive(asSwr(m_storage)); }
bool Mutex::tryLock() { return TryAcquireSRWLockExclusive(asSwr(m_storage)) != 0; }
void Mutex::unlock() { ReleaseSRWLockExclusive(asSwr(m_storage)); }
void* Mutex::native() noexcept { return asSwr(m_storage); }

}  // namespace nme