#ifndef NME_JOB_SYSTEM_H_
#define NME_JOB_SYSTEM_H_

#include <utility>  // std::forward, std::decay_t

#include "nme/core/jobs/job.h"
#include "nme/core/jobs/ws_deque.h"
#include "nme/core/subsystem/subsystem.h"
#include "nme/platform/thread/atomics.h"
#include "nme/platform/thread/condition_variable.h"
#include "nme/platform/thread/mutex.h"
#include "nme/platform/thread/thread.h"
#include "nme/platform/types.h"

namespace nme {

class JobSystem final : public Subsystem {
private:
    static constexpr usize kDequeCapacity = 8192;
    using Deque = WSDeque<Job, kDequeCapacity>;

    // --- lifecycle ---
    Thread*      m_workers = nullptr;  // array of m_workerCount, owned
    Atomic<b32>  m_running{false};  // false => workers exit their loop
    u32          m_workerCount = 0;

    // --- per owner deque ---
    Deque* m_dequeues   = nullptr;
    u32    m_dequeCount = 0;

    // --- idle-worker parking ---
    ConditionVariable m_idleCv;
    Mutex             m_idleMutex;
    Atomic<u32>       m_sleepers{0};

public:
    struct Config {
        u32         workerCount = 0;             // 0 = one per hw core, minus the calling thread
        usize       stackSize   = 0;             // passed through worker thread
        const char* namePrefix  = "nme.worker";  // "nme.worker.0", ".1", ... for Tracy
    };

    JobSystem() = default;
    ~JobSystem() override;   // calls shutdown() if it's still running.

    JobSystem(const JobSystem& other)            = delete;
    JobSystem(JobSystem&& other)                 = delete;
    JobSystem& operator=(const JobSystem& other) = delete;
    JobSystem& operator=(JobSystem&& other)      = delete;

    // Spin up workers. Call once, from the main thread.
    Error startup(const Config& cfg);
    Error startup() override { return startup(Config{}); }   // defaults; Config is complete here

    // Stop accepting, wake all workers, join them. Destructor calls this by default.
    void shutdown() override;

    [[nodiscard]] u32 workerCount() const noexcept { return m_workerCount; }

    // Enqueue one job. If counter is non-null it is incremented here and
    // decremented when the job completes.
    template<typename Fn>
    void run(Fn&& fn, JobCounter* counter = nullptr, const char* name = nullptr) {
        if (counter) counter->add(1);
        enqueue(Job{
            new detail::JobClosure<std::decay_t<Fn>>(std::forward<Fn>(fn)),
            counter,
            name
        });
    }

    // Parallel-for over [0, count): splits into ~kChunksPerWorker*workers chunk
    // jobs, each looping fn(i). Chunking (vs one job per index) avoids overflowing
    // the submitter's deque into enqueue()'s inline path. grain = min items/chunk
    // (0 => auto). Counter tracks chunks, not items.
    template<typename Fn>
    void runN(const u32 count, Fn fn, JobCounter& counter,
              const char* name = nullptr, const u32 grain = 0) {
        if (count == 0) return;

        constexpr u32 kChunksPerWorker = 8;  // > 1 so stealing can rebalance uneven work
        const u32 workers = m_workerCount ? m_workerCount : 1u;
        u32 chunkSize = grain ? grain
                              : (count + workers * kChunksPerWorker - 1) / (workers * kChunksPerWorker);
        if (chunkSize == 0) chunkSize = 1;
        const u32 chunks = (count + chunkSize - 1) / chunkSize;

        counter.add(chunks);  // MUST match the number of jobs enqueued below
        for (u32 c = 0; c < chunks; ++c) {
            const u32 lo = c * chunkSize;
            const u32 hi = (lo + chunkSize < count) ? lo + chunkSize : count;
            auto task = [fn, lo, hi]() mutable {
                for (u32 i = lo; i < hi; ++i) fn(i);
            };
            enqueue(Job{
                new detail::JobClosure<decltype(task)>(std::move(task)),
                &counter,
                name
            });
        }
    }

    // Recursive-split parallel-for: submits one range that bisects down
    // to grain, spawning the right half onto the running worker's deque. Spreads
    // the spawn across deques; counter tracks in-flight jobs. grain = leaf size
    // (0 => auto). Same signature as runN -- swap the call to A/B.
    template<typename Fn>
    void parallelFor(const u32 count, Fn fn, JobCounter& counter,
                     const char* name = nullptr, u32 grain = 0) {
        if (count == 0) return;
        if (grain == 0) {
            constexpr u32 kLeavesPerWorker = 8;
            const u32 workers = m_workerCount ? m_workerCount : 1u;
            grain = (count + workers * kLeavesPerWorker - 1) / (workers * kLeavesPerWorker);
            if (grain == 0) grain = 1;
        }
        // Descend on the calling thread: the left-most leaf runs inline here, the
        // right halves become stealable jobs. Caller then waitForCounter(counter).
        submitRange(0, count, grain, fn, counter, name);
    }

    // Block the CALLING thread until the counter hits zero.
    void waitForCounter(const JobCounter& counter) const;

    [[nodiscard]] const char* name() const override { return "JobSystem"; }

private:
    // Split [lo, hi): spawn the right half (via run(), so it's counted and lands
    // on this thread's deque), halve the left in place, run the leaf. Recursion
    // flows through the queue, not the stack -- this frame is ~log2(range/grain) deep.
    template<typename Fn>
    void submitRange(const u32 lo, u32 hi, const u32 grain, Fn fn,
                     JobCounter& counter, const char* name) {
        while (hi - lo > grain) {
            const u32 mid = lo + (hi - lo) / 2;
            run([this, mid, hi, grain, fn, &counter, name]() {
                    submitRange(mid, hi, grain, fn, counter, name);
                }, &counter, name);
            hi = mid;
        }
        for (u32 i = lo; i < hi; ++i) fn(i);
    }

    void enqueue(const Job& job);              // push to the CALLING thread's own deque
    void destroyDequeues() noexcept;
    bool getJob(u32 self, Job& out) const;    // false if no work available right now
    bool trySteal(u32 self, Job& out) const;  // one random-victim steal sweep

    void wakeOne();                     // wake a parked worker if any
    void wakeAll();                     // shutdown

    static void runJob(const Job& job); // run() + delete closure + decrement counter
    void workerMain(u32 index);         // the per-worker loop
};

}  // namespace nme

#endif  // NME_JOB_SYSTEM_H_