#include "nme/core/jobs/job_system.h"
#include "nme/core/assert/assert.h"
#include "nme/core/debug/profiler.h"

#include <new>     // placement new for deque array
#include <thread>  // std::thread::hardware_concurrency (a query, not a spawn)

namespace nme {

namespace {

// 2^32 / golden-ratio, the standard xorshift/Fibonacci-hash mixing constant
constexpr u32 kGoldenRatio32 = 0x9E3779B9u; // == 265435761

// Distinct, arbitrary base seeds so each thread's victim-selection stream differs
constexpr u32 kWorkerSeedBase = 0x9E3779B9u;
constexpr u32 kMainSeedBase   = 0x01234567u;

constexpr u32 kNoOwner = 0xFFFFFFFFu;

thread_local u32 t_ownerIndex = kNoOwner;
thread_local u32 t_rng        = kGoldenRatio32;

// Cheap per-thread PRNG for random victim selection (xorshift). Seeded per
// thread so workers don't all pick the same victim sequence.
NME_FORCEINLINE u32 nextRand() noexcept {
    u32 x = t_rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    t_rng = x;
    return x;
}

}  // namespace

JobSystem::~JobSystem() {
    shutdown();
}

void JobSystem::startup(const Config& cfg) {
    NME_ASSERT(!m_running.load(MemoryOrder::Acquire));  // startup once

    u32 count = cfg.workerCount;
    if (count == 0) {
        const unsigned hw = std::thread::hardware_concurrency();
        count = (hw > 1) ? static_cast<u32>(hw - 1) : 1u;
    }

    m_workerCount = count;
    m_dequeCount  = count + 1;

    m_dequeues = static_cast<Deque*>(::operator new [](sizeof(Deque) * m_dequeCount));
    for (u32 i = 0; i < m_dequeCount; ++i) {
        ::new (&m_dequeues[i]) Deque();
    }

    // the main thread owns the last deque; that's how run/runN from main knows
    // where to push (via t_ownerIndex in enqueue).
    t_ownerIndex = m_workerCount;
    t_rng = kMainSeedBase ^ (m_workerCount * kGoldenRatio32);

    m_running.store(true, MemoryOrder::Release);

    m_workers = static_cast<Thread*>(::operator new[](sizeof(Thread) * count));
    for (u32 i = 0; i < count; ++i) {
        char name[32];
        // small, dependency-free formatting: "<prefix>.<i>"
        int n = 0;
        for (const char* p = cfg.namePrefix; *p && n < 27; ++p) name[n++] = *p;
        name[n++] = '.';
        if (i >= 10) name[n++] = static_cast<char>('0' + (i / 10) % 10);
        name[n++] = static_cast<char>('0' + i % 10);
        name[n]   = '\0';

        ThreadConfig tc{};
        tc.name      = name;
        tc.stackSize = cfg.stackSize;
        tc.affinity  = static_cast<i32>(i);  // pin worker i to core i (visible in Tracy)

        ::new (&m_workers[i]) Thread([this, i]() { workerMain(i); }, tc);
    }
}
void JobSystem::shutdown() {
    if (!m_running.load(MemoryOrder::Acquire)) return;

    m_running.store(false, MemoryOrder::Release);
    wakeAll();

    for (u32 i = 0; i < m_workerCount; ++i) {
        if (m_workers[i].joinable()) m_workers[i].join();
        m_workers[i].~Thread();
    }
    ::operator delete [](m_workers);
    m_workers     = nullptr;
    m_workerCount = 0;

    // drain and free any closures left unrun
    for (u32 d = 0; d < m_dequeCount; ++d) {
        Job j;
        while (m_dequeues[d].pop(j)) delete j.closure;
        m_dequeues[d].~Deque();
    }
    ::operator delete [](m_dequeues);
    m_dequeues    = nullptr;
    m_dequeCount  = 0;
    m_workerCount = 0;
}

void JobSystem::enqueue(const Job& job) {
    // push to the CALLING thread's own deque
    NME_ASSERT(t_ownerIndex != kNoOwner && "enqueue from a thread that owns no deque");
    NME_ASSERT(t_ownerIndex < m_dequeCount);

    if (!m_dequeues[t_ownerIndex].push(job)) {
        // deque full, run the job inline
        runJob(job);
        return;
    }

    // wake a parked worder if any are sleeping
    if (m_sleepers.load(MemoryOrder::Acquire) != 0) {
        wakeOne();
    }
}

bool JobSystem::trySteal(const u32 self, Job& out) const {
    const u32 n = m_dequeCount;
    const u32 attempts = n * 2;
    for (u32 k = 0; k < attempts; ++k) {
        const u32 victim = nextRand() % n;
        if (victim == self) continue;
        switch (m_dequeues[victim].steal(out)) {
            case Deque::StealResult::Success: return true;
            case Deque::StealResult::Abort:         // lost a trace; try again
            case Deque::StealResult::Empty: break;  // nothing here; next victim
        }
    }
    return false;
}

bool JobSystem::getJob(const u32 self, Job& out) const {
    // Fast path: own deque
    if (self != kNoOwner && self < m_dequeCount && m_dequeues[self].pop(out)) {
        return true;
    }
    // Slow path: steal from someone
    return trySteal(self, out);
}

void JobSystem::wakeOne() { m_idleMutex.lock(); m_idleCv.notifyOne(); m_idleMutex.unlock(); }
void JobSystem::wakeAll() { m_idleMutex.lock(); m_idleCv.notifyAll(); m_idleMutex.unlock(); }

void JobSystem::runJob(const Job& job) {
    NME_PROFILE_ZONE();
    NME_PROFILE_ZONE_TEXT(job.name ? job.name : "job");

    job.closure->run();
    delete job.closure;
    if (job.counter) job.counter->decrement();
}

void JobSystem::workerMain(const u32 index) {
    // Register this worker's identity
    t_ownerIndex = index;
    t_rng = kWorkerSeedBase ^ (index * kGoldenRatio32 + 1u);

    // Tracy row label.
    char name[24];
    int n = 0;
    for (auto p = "nme.worker."; *p; ++p) name[n++] = *p;
    if (index >= 10) name[n++] = static_cast<char>('0' + (index / 10) % 10);
    name[n++] = static_cast<char>('0' + index % 10);
    name[n] = '\0';
    NME_PROFILE_THREAD_NAME(name);

    while (m_running.load(MemoryOrder::Acquire)) {
        Job job;
        if (getJob(index, job)) {
            runJob(job);
            continue;
        }

        // Nothing found after a steal sweep, park until woken or shut down
        m_idleMutex.lock();
        m_sleepers.fetchAdd(1, MemoryOrder::AcqRel);
        // One more steal attempt whilst holding "I'm about to sleep" state
        if (getJob(index, job)) {
            m_sleepers.fetchSub(1, MemoryOrder::AcqRel);
            m_idleMutex.unlock();
            runJob(job);
            continue;
        }
        if (m_running.load(MemoryOrder::Acquire)) {
            m_idleCv.wait(m_idleMutex);
        }
        m_sleepers.fetchSub(1, MemoryOrder::AcqRel);
        m_idleMutex.unlock();
    }
}

void JobSystem::waitForCounter(const JobCounter& counter) const {
    // help drain by stealing: keep the calling (usually main) thread productive
    // while the phase completes. also drains the main thread's own submission
    // deque, which nothing else pops.
    const u32 self = t_ownerIndex;
    while (!counter.done()) {
        Job job;
        if (getJob(self, job)) {
            runJob(job);
        } else {
            yieldCurrentThread();  // nothing to steal right now
        }
    }
}

}  // namespace nme