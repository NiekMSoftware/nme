//==============================================================================
// job_bench -- job system throughput + Tracy driver
//------------------------------------------------------------------------------
// Two modes:
//
//   Throughput (default): dispatch N identical jobs per frame, report jobs/sec.
//     job_bench [--jobs N] [--work-per-job W] [--frames F]
//
//   Frame simulation: pretend to be an engine. Distinct job TYPES with
//     different costs and counts, run as dependency PHASES separated by
//     barriers (anim -> physics -> culling -> render prep). This is the
//     bulk-synchronous model 4c supports: each phase runN's, then
//     waitForCounter's before the next begins. In Tracy you SEE the barriers
//     (all workers converge, a gap, the next phase fans out) and the distinct
//     zone names per phase. The render "main pass" job is deliberately fat so
//     you get a realistic straggler that stalls the barrier.
//     job_bench --frame [--frames F] [--scale S]
//
//   --verify   run the correctness asserts instead.
//==============================================================================
#include <cassert>
#include <cstdio>
#include <cstring>   // std::strcmp
#include <cstdlib>   // std::strtoul

#include "nme/core/jobs/job_system.h"
#include "nme/core/debug/profiler.h"
#include "nme/platform/thread/atomics.h"
#include "nme/platform/timer/timer.h"
#include "nme/platform/types.h"

namespace {

// A tiny, un-optimizable-away compute kernel. Returns a value the caller sinks
// so the loop can't be elided. `work` scales the per-job cost.
nme::u64 busyWork(const nme::u32 work, const nme::u32 seed) {
    nme::u64 acc = seed;
    for (nme::u32 i = 0; i < work; ++i) {
        acc = acc * 6364136223846793005ULL + 1442695040888963407ULL;  // LCG step
        acc ^= acc >> 17;
    }
    return acc;
}

// Global sink: every job folds its result in so the compiler can't elide the
// work. Relaxed -- we only need the writes to happen, not to be ordered.
nme::Atomic<nme::u64> g_sink{0};
inline void sink(const nme::u64 v) { g_sink.fetchAdd(v, nme::MemoryOrder::Relaxed); }

struct Args {
    nme::u32 jobs        = 10'000;
    nme::u32 workPerJob  = 2'000;
    nme::u32 frames      = 200;
    nme::u32 scale       = 1;       // frame-sim: multiplies entity counts
    bool     verify      = false;
    bool     frame       = false;   // run the engine-frame simulation
};

Args parse(const int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        auto next = [&](const nme::u32 def) -> nme::u32 {
            return (i + 1 < argc) ? static_cast<nme::u32>(std::strtoul(argv[++i], nullptr, 10)) : def;
        };
        if      (std::strcmp(argv[i], "--jobs")         == 0) a.jobs       = next(a.jobs);
        else if (std::strcmp(argv[i], "--work-per-job") == 0) a.workPerJob = next(a.workPerJob);
        else if (std::strcmp(argv[i], "--frames")       == 0) a.frames     = next(a.frames);
        else if (std::strcmp(argv[i], "--scale")        == 0) a.scale      = next(a.scale);
        else if (std::strcmp(argv[i], "--frame")        == 0) a.frame      = true;
        else if (std::strcmp(argv[i], "--verify")       == 0) a.verify     = true;
    }
    if (a.scale == 0) a.scale = 1;
    return a;
}

// -----------------------------------------------------------------------------
// Correctness mode
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
    js.runN(kJobCount, [](const u32 i) { slots[i] = i * 2 + 1; }, counter2);
    js.waitForCounter(counter2);
    for (u32 i = 0; i < kJobCount; ++i) {
        assert(slots[i] == i * 2 + 1 && "each slot must be written by its job");
    }

    std::printf("PASS\n");
    return 0;
}

// -----------------------------------------------------------------------------
// Throughput mode (uniform jobs)
// -----------------------------------------------------------------------------
int runThroughput(nme::JobSystem& js, const Args& args, nme::platform::Timer& timer) {
    using namespace nme;

    std::printf("bench: jobs=%u work-per-job=%u frames=%u\n",
                args.jobs, args.workPerJob, args.frames);

    const u64 tStart = timer.now();
    u64 totalJobs = 0;

    for (u32 f = 0; f < args.frames; ++f) {
        NME_PROFILE_FRAME_MARK();

        JobCounter counter;
        const u32 work = args.workPerJob;
        js.runN(args.jobs, [work](const u32 i) { sink(busyWork(work, i)); },
                counter, "bench.work");
        js.waitForCounter(counter);

        totalJobs += args.jobs;
    }

    const f64 seconds = timer.to_seconds(timer.now() - tStart);
    std::printf("jobs:      %llu\n", static_cast<unsigned long long>(totalJobs));
    std::printf("sink:      %llu\n", static_cast<unsigned long long>(g_sink.load(MemoryOrder::Relaxed)));
    if (seconds > 0.0) {
        std::printf("elapsed:   %8.3f s\n", seconds);
        std::printf("throughput:%12.0f jobs/s\n", static_cast<f64>(totalJobs) / seconds);
        std::printf("per job:   %8.1f ns\n", (seconds * 1e9) / static_cast<f64>(totalJobs));
    } else {
        std::printf("elapsed:   0.000 s (below timer resolution -- raise --frames)\n");
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Frame-simulation mode: pretend to be an engine.
// -----------------------------------------------------------------------------
// One phase = one job type. runN then waitForCounter = a dependency barrier.
// Counts scale with --scale; per-job work is tuned so the phases have visibly
// different weights, the way real subsystems do.
int runFrameSim(nme::JobSystem& js, const Args& args, nme::platform::Timer& timer) {
    using namespace nme;

    const u32 s = args.scale;
    const u32 kAnim     = 2000 * s;   // many small jobs: one per skinned mesh
    const u32 kPhysics  =  256 * s;   // fewer, heavier: solver islands
    const u32 kCulling  =  512 * s;   // medium: frustum test per object batch
    const u32 kRender   =   64 * s;   // few big: command-buffer builds

    constexpr u32 wAnim     =   800;      // light
    constexpr u32 wPhysics  =  6000;      // heavy
    constexpr u32 wCulling  =  1200;      // medium
    constexpr u32 wRender   =  4000;      // heavy

    std::printf("frame-sim: scale=%u frames=%u\n", s, args.frames);
    std::printf("  anim=%u(w%u) physics=%u(w%u) culling=%u(w%u) render=%u(w%u)\n",
                kAnim, wAnim, kPhysics, wPhysics, kCulling, wCulling, kRender, wRender);

    const u64 tStart = nme::platform::Timer::now();
    u64 totalJobs = 0;
    u64 worstFrameTicks = 0;

    for (u32 f = 0; f < args.frames; ++f) {
        NME_PROFILE_FRAME_MARK();
        const u64 frameStart = nme::platform::Timer::now();

        // --- Phase 1: animation (depends on nothing) ---
        {
            JobCounter c;
            js.runN(kAnim, [wAnim](const u32 i) { sink(busyWork(wAnim, i)); }, c, "anim.pose");
            js.waitForCounter(c);
        }
        // --- Phase 2: physics (reads animated transforms) ---
        {
            JobCounter c;
            js.runN(kPhysics, [wPhysics](const u32 i) { sink(busyWork(wPhysics, i)); }, c, "physics.solve");
            js.waitForCounter(c);
        }
        // --- Phase 3: culling (reads final transforms) ---
        {
            JobCounter c;
            js.runN(kCulling, [wCulling](const u32 i) { sink(busyWork(wCulling, i)); }, c, "cull.frustum");
            js.waitForCounter(c);
        }
        // --- Phase 4: render prep (reads visible set) ---
        // Includes ONE deliberately fat "main pass" job: a realistic straggler
        // that holds the barrier open while everything else is done. Watch it
        // in Tracy -- the whole phase can't complete until this one finishes.
        {
            JobCounter c;
            js.runN(kRender, [wRender](const u32 i) { sink(busyWork(wRender, i)); }, c, "render.cmdbuf");
            js.run([]() { sink(busyWork(60000, 7)); }, &c, "render.mainpass");  // the straggler
            js.waitForCounter(c);
        }

        totalJobs += kAnim + kPhysics + kCulling + kRender + 1;

        const u64 ft = timer.now() - frameStart;
        if (ft > worstFrameTicks) worstFrameTicks = ft;
    }

    const f64 seconds  = timer.to_seconds(timer.now() - tStart);
    const f64 avgFrame = (seconds * 1000.0) / static_cast<f64>(args.frames);
    const f64 worstMs  = timer.to_seconds(worstFrameTicks) * 1000.0;

    std::printf("jobs:      %llu\n", static_cast<unsigned long long>(totalJobs));
    std::printf("sink:      %llu\n", static_cast<unsigned long long>(g_sink.load(MemoryOrder::Relaxed)));
    std::printf("elapsed:   %8.3f s\n", seconds);
    std::printf("avg frame: %8.3f ms  (%.1f fps)\n", avgFrame, 1000.0 / avgFrame);
    std::printf("worst frame:%7.3f ms  <- the straggler's frame\n", worstMs);
    return 0;
}

}  // namespace

int main(const int argc, char** argv) {
    using namespace nme;

    const Args args = parse(argc, argv);

    JobSystem js;
    js.startup();
    std::printf("workers: %u\n", js.workerCount());

    platform::Timer& timer = platform::global_timer();
    timer.startup();

    int rc = 0;
    if      (args.verify) rc = runVerify(js);
    else if (args.frame)  rc = runFrameSim(js, args, timer);
    else                  rc = runThroughput(js, args, timer);

    timer.shutdown();
    js.shutdown();
    return rc;
}