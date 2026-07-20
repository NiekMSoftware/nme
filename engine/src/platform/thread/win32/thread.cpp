#include "nme/platform/thread/thread.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <process.h>  // _beginthreadex

namespace nme
{

// Uniform closure -> the CRT thread signature. Use _beginthreadex, NOT
// CreateThread: it initializes per-thread CRT state, so callables that touch
// the C runtime don't leak small resources.
static unsigned __stdcall threadTrampoline(void *arg) {
    auto* closure = static_cast<detail::ThreadClosureBase*>(arg);
    closure->run();
    delete closure;    // closure owns itself; freed when body ends
    return 0;
}

void Thread::start(detail::ThreadClosureBase *closure, const ThreadConfig &cfg) {
    unsigned tid = 0;
    // Create suspended when we must pin before the first instruction runs.
    const unsigned flags = (cfg.affinity >= 0) ? CREATE_SUSPENDED : 0;
    const uintptr_t h = _beginthreadex(nullptr, 
                                       static_cast<unsigned>(cfg.stackSize),
                                       &threadTrampoline, closure, flags, &tid);

    if (h == 0) {
        delete closure;              // no thread was created to free it
        NME_ASSERT(false);  // thread creation failing is fatal
        return;
    }
    m_handle = reinterpret_cast<void*>(h);
    m_id     = tid;

    if (cfg.name) setName(cfg.name);
    if (cfg.affinity >= 0) {
        SetThreadAffinityMask(static_cast<HANDLE>(m_handle), 1ull << cfg.affinity);
        ResumeThread(static_cast<HANDLE>(m_handle));
    }
}

void Thread::join() {
    NME_ASSERT(joinable());
    WaitForSingleObject(static_cast<HANDLE>(m_handle), INFINITE);
    CloseHandle(static_cast<HANDLE>(m_handle));  // _beginthreadex handle must be closed
    m_handle = nullptr; m_id = 0;
}

void Thread::detach() {
    NME_ASSERT(joinable());
    CloseHandle(static_cast<HANDLE>(m_handle));
    m_handle = nullptr; m_id = 0;
}

Thread::~Thread() {
    NME_ASSERT(!joinable());
}

void Thread::setAffinity(const u32 coreIndex) const {
    SetThreadAffinityMask(static_cast<HANDLE>(m_handle), 1ull << coreIndex);
}

void Thread::setName(const char *name) const {
    if (!name || !m_handle) return;
    wchar_t wide[256];
    const int n = MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, 256);
    if (n > 0) {
        // check if we are on an older Windows sdk to set the thread description
        using SetThreadDescFn = HRESULT (WINAPI*)(HANDLE, PCWSTR);
        static auto pSetThreadDesc = reinterpret_cast<SetThreadDescFn>(
            GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription"));
        if (pSetThreadDesc) SetThreadDescription(static_cast<HANDLE>(m_handle), wide); 
        // older Windows SDKs will just fail, which is fine
    }
    // n == 0 meant it didn't fit, so it's fine to skip; it's only a label
}

ThreadId currentThreadId()                { return GetCurrentThreadId(); }
void     yieldCurrentThread()             { SwitchToThread(); }
void     sleepCurrentThread(const u64 ms) { Sleep(static_cast<DWORD>(ms)); }

}  // namespace nme
