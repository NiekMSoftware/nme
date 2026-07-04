#ifndef NME_JOB_SYSTEM_H_
#define NME_JOB_SYSTEM_H_

#include <deque>        // will be replaced with per-worker Chase-Lev deque
#include <utility>      // std::forward, std::decay_t

#include "nme/core/jobs/job.h"
#include "nme/platform/thread/atomics.h"
#include "nme/platform/thread/condition_variable.h"
#include "nme/platform/thread/mutex.h"
#include "nme/platform/thread/thread.h"
#include "nme/platform/types.h"

namespace nme {

class JobSystem {
private:
    // --- lifecycle ---
    Thread*      m_workers = nullptr;  // array of m_workerCount, owned
    Atomic<b32>  m_running{false};  // false => workers exit their loop
    u32          m_workerCount = 0;

    // --- shared queue ---
    ConditionVariable m_queueCv;
    Mutex             m_queueMutex;
    std::deque<Job>   m_queue;

public:
    struct Config {
        u32         workerCount = 0;             // 0 = one per hw core, minus the calling thread
        usize       stackSize   = 0;             // passed through worker thread
        const char* namePrefix  = "nme.worker";  // "nme.worker.0", ".1", ... for Tracy
    };

    JobSystem() = default;
    ~JobSystem();   // calls shutdown() if it's still running.

    JobSystem(const JobSystem& other)            = delete;
    JobSystem(JobSystem&& other)                 = delete;
    JobSystem& operator=(const JobSystem& other) = delete;
    JobSystem& operator=(JobSystem&& other)      = delete;

    // Spin up workers. Call once, from the main thread.
    void startup(const Config& cfg);
    void startup() { startup(Config{}); }   // defaults; Config is complete here

    // Stop accepting, wake all workers, join them. Destructor calls this by default.
    void shutdown();

    [[nodiscard]] u32 workerCount() const noexcept { return m_workerCount; }

    // Enqueue one job. If counter is non-null it is incremented here and
    // decremented when the job completes.
    template<typename Fn>
    void run(Fn&& fn, JobCounter* counter = nullptr) {
        if (counter) counter->add(1);
        enqueue(Job{
            new detail::JobClosure<std::decay_t<Fn>>(std::forward<Fn>(fn)),
            counter
        });
    }

    // Enqueue N jobs against one counter, each invoked as fn(i).
    template<typename Fn>
    void runN(const u32 count, Fn fn, JobCounter& counter) {
        counter.add(count);
        for (u32 i = 0; i < count; ++i) {
            auto task = [fn, i]() mutable { fn(i); };
            enqueue(Job{
                new detail::JobClosure<decltype(task)>(std::move(task)),
                &counter
            });
        }
    }

    // Block the CALLING thread until the counter hits zero.
    void waitForCounter(JobCounter& counter);

private:
    void enqueue(Job job);
    bool getJob(u32 self, Job& out);    // false if no work available right now

    static void runJob(const Job& job); // run() + delete closure + decrement counter
    void workerMain(u32 index);         // the per-worker loop
};

}  // namespace nme

#endif  // NME_JOB_SYSTEM_H_
