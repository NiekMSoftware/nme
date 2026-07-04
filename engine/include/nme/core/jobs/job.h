#ifndef NME_JOB_H_
#define NME_JOB_H_

#include "nme/platform/types.h"
#include "nme/platform/thread/atomics.h"

#include <utility>  // std::move

namespace nme {

class JobCounter {
private:
    Atomic<u32> m_pending{0};

public:
    JobCounter() = default;
    explicit JobCounter(const u32 initial) noexcept : m_pending(initial) {}

    JobCounter(const JobCounter&)            = delete;
    JobCounter& operator=(const JobCounter&) = delete;

    // Producer side, before dispatch. Relaxed: the actual publish/release of
    // the job to workers happens on the queue push, this just needs atomicity.
    void add(const u32 n) noexcept { m_pending.fetchAdd(n, MemoryOrder::Relaxed); }

    // Worker side, when a job finished. Release so the waiter (which acquires)
    // sees all the job's memory writes. Returns the value BEFORE to
    // subtract, so `== 1` means "I was the last one to finish".
    u32 decrement() noexcept { return m_pending.fetchSub(1, MemoryOrder::Release); }

    [[nodiscard]] u32 load() const noexcept { return m_pending.load(MemoryOrder::Acquire); }
    [[nodiscard]] bool done() const noexcept { return load() == 0; }
};

namespace detail {
struct JobClosureBase {
    virtual void run() = 0;
    virtual ~JobClosureBase() = default;
};
template<typename T>
struct JobClosure final : JobClosureBase {
    T fn;
    explicit JobClosure(T fn) noexcept : fn(std::move(fn)) {}
    void run() override { fn(); }
};
}  // namespace detail

// What actually sits in the queue: a type-erased callable plus the counter it
// decrements on completion (nullptr for fire-and-forget).
struct Job {
    detail::JobClosureBase* closure = nullptr;  // owned by the JobSystem
    JobCounter*             counter = nullptr;  // decremented after run(), may be null
};

}  // namespace nme

#endif  // NME_JOB_H_
