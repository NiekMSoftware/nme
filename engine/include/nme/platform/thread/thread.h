#pragma once

#include "nme/platform/types.h"
#include "nme/platform/debug/assert.h" // NME_PLATFORM_ASSERT

#include <utility>  // std::move, std::forward, std::decay_t

namespace nme {

using ThreadId = u64;

struct ThreadConfig {
    const char* name      = nullptr;    // debugger / Tracy label
    usize       stackSize = 0;          // 0 = OS default
    i32         affinity  = -1;         // -1 = unpinned, else score index
};

namespace detail
{
// Type erasure: one closure type per callable, all reachable through a
// uniform base. Lives in detail so the .cpp trampoline can name the base.
struct ThreadClosureBase {
    virtual void run() = 0;
    virtual ~ThreadClosureBase() = default;
};

template<typename T>
struct ThreadClosure final : ThreadClosureBase {
    T fn;
    explicit ThreadClosure(T f) : fn(std::move(f)) {}
    void run() override { fn(); }
};
}  // namespace detail

class Thread {
private:
    void* m_handle = nullptr;
    ThreadId m_id = 0;

public:
    Thread() = default;

    template<typename Fn>
    explicit Thread(Fn &&fn, const ThreadConfig &cfg = {}) {
        start(new detail::ThreadClosure<std::decay_t<Fn>>(std::forward<Fn>(fn)), cfg);
    }

    ~Thread();

    Thread(const Thread&)            = delete;
    Thread& operator=(const Thread&) = delete;

    // Move-only
    Thread(Thread&& o) noexcept : m_handle(o.m_handle), m_id(o.m_id) {
        o.m_handle = nullptr; o.m_id = 0;
    }
    Thread& operator=(Thread &&o) noexcept { 
        if (this != &o) {
            NME_PLATFORM_ASSERT(!joinable());
            m_handle = o.m_handle; m_id = o.m_id;
            o.m_handle = nullptr; o.m_id = 0;
        }
        return *this;
    }

    void join();
    void detach();

    [[nodiscard]] bool joinable() const { return m_handle != nullptr; }  // trivial, portable
    [[nodiscard]] ThreadId id() const { return m_id; }

    // The reason this wrapper exists over std::thread - none of these are
    // portably exposed by the standard library.
    void setAffinity(u32 coreIndex) const;
    void setName(const char* name) const;

private:
    void start(detail::ThreadClosureBase* closure, const ThreadConfig& cfg);
};

ThreadId currentThreadId();
void     yieldCurrentThread();
void     sleepCurrentThread(u64 ms);

}  // namespace nme
