//==============================================================================
// job_bench -- job system throughput + Tracy driver
//------------------------------------------------------------------------------
// Usage:
//   job_bench [--jobs N] [--work-per-job W] [--frames F] [--verify]
//
//   --jobs N          number of jobs dispatched per frame   (default 100000)
//   --work-per-job W  busy-work iterations inside each job   (default 0 = trivial)
//   --frames F        dispatch/wait cycles to run            (default 200)
//   --verify          run the correctness asserts instead of the throughput loop
//
// Read it in Tracy: each frame is a FrameMark; each job is a "job" zone; worker
// rows are "nme.worker.N". With --work-per-job 0 you should SEE the shared-queue
// ceiling -- workers spend more time contending on the queue mutex than working,
// so the rows look sparse and lopsided. Raise --work-per-job (e.g. 2000) and the
// rows fill and spread evenly across cores: that is real parallelism, and the
// contrast is the empirical case for 4e's per-worker deques.
//==============================================================================
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "nme/core/jobs/job_system.h"
#include "nme/core/debug/profiler.h"
#include "nme/platform/thread/atomics.h"
#include "nme/platform/timer/timer.h"
#include "nme/platform/types.h"

namespace {

// A tiny, un-optimizable-away compute kernel. Returns a value the caller sinks
// so the loop can't be elided. `work` scales the per-job cost.
nme::u64 busyWork(nme::u32 work, nme::u32 seed) {
    nme::u64 acc = seed;
    for (nme::u32 i = 0; i < work; ++i) {
        acc = acc * 6364136223846793005ULL + 1442695040888963407ULL;  // LCG step
        acc ^= acc >> 17;
    }
    return acc;
}

struct Args {
    nme::u32 jobs        = 10'000;
    nme::u32 workPerJob  = 0;
    nme::u32 frames      = 200;
    bool     verify      = false;
};

Args parse(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        auto next = [&](nme::u32 def) -> nme::u32 {
            return (i + 1 < argc) ? static_cast<nme::u32>(std::strtoul(argv[++i], nullptr, 10)) : def;
        };
        if      (std::strcmp(argv[i], "--jobs")         == 0) a.jobs       = next(a.jobs);
        else if (std::strcmp(argv[i], "--work-per-job") == 0) a.workPerJob = next(a.workPerJob);
        else if (std::strcmp(argv[i], "--frames")       == 0) a.frames     = next(a.frames);
        else if (std::strcmp(argv[i], "--verify")       == 0) a.verify     = true;
    }
    return a;
}

// -----------------------------------------------------------------------------
// Correctness mode: the original 4c smoke test, kept as `--verify`.
// -----------------------------------------------------------------------------
int runVerify(nme::JobSystem& js) {
    using namespace nme;
    constexpr u32 kJobCount = 10'000;

    Atomic<u32> ran{0};
    JobCounter counter;
    js.runN(kJobCount, [&ran](u32) { ran.fetchAdd(1, MemoryOrder::Relaxed); }, counter);
    js.waitForCounter(counter);

    const u32 total = ran.load(MemoryOrder::Acquire);
    std::printf("ran: %u / %u\n", total, kJobCount);
    assert(total == kJobCount && "every job must run exactly once");
    assert(counter.done() && "counter must reach zero");

    static u32 slots[kJobCount] = {};
    JobCounter counter2;
    js.runN(kJobCount, [](u32 i) { slots[i] = i * 2 + 1; }, counter2);
    js.waitForCounter(counter2);
    for (u32 i = 0; i < kJobCount; ++i) {
        assert(slots[i] == i * 2 + 1 && "each slot must be written by its job");
    }

    std::printf("PASS\n");
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace nme;

    const Args args = parse(argc, argv);

    JobSystem js;
    js.startup();
    std::printf("workers: %u\n", js.workerCount());

    if (args.verify) {
        const int rc = runVerify(js);
        js.shutdown();
        return rc;
    }

    // A sink per job avoids the optimizer deleting busyWork, without any
    // cross-job contention (each job writes its own slot).
    static Atomic<u64> g_sink{0};

    platform::Timer& timer = platform::global_timer();
    timer.startup();

    std::printf("bench: jobs=%u work-per-job=%u frames=%u\n",
                args.jobs, args.workPerJob, args.frames);

    const u64 tStart = platform::Timer::now();
    u64 totalJobs = 0;

    for (u32 f = 0; f < args.frames; ++f) {
        NME_PROFILE_FRAME_MARK();

        JobCounter counter;
        const u32 work = args.workPerJob;
        js.runN(args.jobs, [work](u32 i) {
            const u64 r = busyWork(work, i);
            // relaxed: we only need to write not to be elided, not ordered
            g_sink.fetchAdd(r, MemoryOrder::Relaxed);
        }, counter);
        js.waitForCounter(counter);

        totalJobs += args.jobs;
    }

    const u64 tEnd      = platform::Timer::now();
    const u64 rawTicks  = tEnd - tStart;
    const f64 seconds   = timer.to_seconds(rawTicks);

    std::printf("raw ticks: %llu -> %llu (delta %llu)\n",
                static_cast<unsigned long long>(tStart),
                static_cast<unsigned long long>(tEnd),
                static_cast<unsigned long long>(rawTicks));
    std::printf("jobs:      %llu\n", static_cast<unsigned long long>(totalJobs));
    std::printf("sink:      %llu\n",  // printed so g_sink can't be dead-code-eliminated
                static_cast<unsigned long long>(g_sink.load(MemoryOrder::Relaxed)));
    const f64 jobsPerSec = static_cast<f64>(totalJobs) / seconds;
    const f64 nsPerJob   = (seconds * 1e9) / static_cast<f64>(totalJobs);
    std::printf("elapsed:   %8.3f s\n", seconds);
    std::printf("throughput:%12.0f jobs/s\n", jobsPerSec);
    std::printf("per job:   %8.1f ns\n", nsPerJob);

    timer.shutdown();
    js.shutdown();
    return 0;
}