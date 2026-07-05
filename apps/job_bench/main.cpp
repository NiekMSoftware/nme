//==============================================================================
// job_bench -- job-system throughput / scaling benchmark + Tracy driver
//------------------------------------------------------------------------------
// Modes (default = throughput):
//
//   Throughput:   dispatch N jobs/frame of a fixed size, report jobs/sec.
//     job_bench [--jobs N] [--work W] [--frames F] [--repeats R]
//
//   Sweep:        the 4e deliverable. Runs a granularity ladder (many tiny
//                 jobs -> few fat jobs) in one invocation and prints a table.
//                 This is the "many small jobs vs single-queue" comparison:
//                 run it on the stealing build and the single-queue build,
//                 diff the tables. Small-job rows are where stealing should win.
//     job_bench --sweep [--frames F] [--repeats R]
//
//   Frame sim:    fake engine frame -- distinct job types as dependency phases
//                 (anim -> physics -> culling -> render), one fat straggler.
//     job_bench --frame [--scale S] [--frames F]
//
//   Verify:       correctness asserts (exactly-once + per-slot writes).
//     job_bench --verify
//
// Benchmark source file has been generated using an LLM.
//==============================================================================
#include <cassert>
#include <cstdio>
#include <algorithm> // std::sort, std::nth_element

#include "nme/core/jobs/job_system.h"
#include "nme/core/debug/profiler.h"
#include "nme/platform/thread/atomics.h"
#include "nme/platform/timer/timer.h"
#include "nme/platform/types.h"

namespace {

using nme::u32;
using nme::u64;
using nme::f64;

// A tiny, un-optimizable-away compute kernel. `work` scales per-job cost.
u64 busyWork(const u32 work, const u32 seed) {
    u64 acc = seed;
    for (u32 i = 0; i < work; ++i) {
        acc = acc * 6364136223846793005ULL + 1442695040888963407ULL;  // LCG step
        acc ^= acc >> 17;
    }
    return acc;
}

// Global sink so the compiler can't elide the work. Relaxed: we only need the
// writes to happen, not to be ordered.
nme::Atomic<u64> g_sink{0};
inline void sink(const u64 v) { g_sink.fetchAdd(v, nme::MemoryOrder::Relaxed); }

struct Args {
    u32  jobs     = 100'000;
    u32  work     = 0;        // 0 = trivial jobs: the fine-grained stress case
    u32  frames   = 100;
    u32  repeats  = 5;        // median over R runs for stability
    u32  scale    = 1;        // frame-sim entity multiplier
    bool sweep    = true;
    bool frame    = false;
    bool verify   = false;
};

Args parse(const int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        auto next = [&](const u32 def) -> u32 {
            return (i + 1 < argc) ? static_cast<u32>(std::strtoul(argv[++i], nullptr, 10)) : def;
        };
        if      (std::strcmp(argv[i], "--jobs")    == 0) a.jobs    = next(a.jobs);
        else if (std::strcmp(argv[i], "--work")    == 0) a.work    = next(a.work);
        else if (std::strcmp(argv[i], "--frames")  == 0) a.frames  = next(a.frames);
        else if (std::strcmp(argv[i], "--repeats") == 0) a.repeats = next(a.repeats);
        else if (std::strcmp(argv[i], "--scale")   == 0) a.scale   = next(a.scale);
        else if (std::strcmp(argv[i], "--sweep")   == 0) a.sweep   = true;
        else if (std::strcmp(argv[i], "--frame")   == 0) a.frame   = true;
        else if (std::strcmp(argv[i], "--verify")  == 0) a.verify  = true;
    }
    if (a.scale   == 0) a.scale   = 1;
    if (a.repeats == 0) a.repeats = 1;
    return a;
}

// -----------------------------------------------------------------------------
// One timed measurement: dispatch `jobs` jobs of size `work`, `frames` times,
// waiting each frame. Returns nanoseconds-per-job (wall clock).
// -----------------------------------------------------------------------------
f64 measureNsPerJob(nme::JobSystem& js, const nme::platform::Timer& timer,
                    const u32 jobs, const u32 work, const u32 frames) {
    const u64 tStart = nme::platform::Timer::now();
    for (u32 f = 0; f < frames; ++f) {
        NME_PROFILE_FRAME_MARK();
        nme::JobCounter counter;
        js.runN(jobs, [work](const u32 i) { sink(busyWork(work, i)); }, counter, "bench.work");
        js.waitForCounter(counter);
    }
    const u64 ticks   = nme::platform::Timer::now() - tStart;
    const f64 seconds = timer.to_seconds(ticks);
    const u64 total   = static_cast<u64>(jobs) * frames;
    if (seconds <= 0.0 || total == 0) return 0.0;
    return (seconds * 1e9) / static_cast<f64>(total);
}

// Median of `repeats` measurements -- robust to the occasional scheduler blip.
f64 medianNsPerJob(nme::JobSystem& js, nme::platform::Timer& timer,
                   const u32 jobs, const u32 work, const u32 frames, const u32 repeats) {
    f64 samples[64];
    const u32 r = (repeats > 64) ? 64 : repeats;
    for (u32 i = 0; i < r; ++i) {
        samples[i] = measureNsPerJob(js, timer, jobs, work, frames);
    }
    std::sort(samples, samples + r);
    return (r & 1) ? samples[r / 2] : 0.5 * (samples[r / 2 - 1] + samples[r / 2]);
}

// -----------------------------------------------------------------------------
// Correctness
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
// Throughput (single config)
// -----------------------------------------------------------------------------
int runThroughput(nme::JobSystem& js, const Args& a, nme::platform::Timer& timer) {
    std::printf("throughput: jobs=%u work=%u frames=%u repeats=%u workers=%u\n",
                a.jobs, a.work, a.frames, a.repeats, js.workerCount());
    const f64 ns = medianNsPerJob(js, timer, a.jobs, a.work, a.frames, a.repeats);
    if (ns <= 0.0) {
        std::printf("  (below timer resolution -- raise --frames/--jobs)\n");
        return 0;
    }
    std::printf("  median: %8.1f ns/job   %12.0f jobs/s\n", ns, 1e9 / ns);
    std::printf("  sink:   %llu\n", static_cast<unsigned long long>(g_sink.load(nme::MemoryOrder::Relaxed)));
    return 0;
}

// -----------------------------------------------------------------------------
// Sweep -- the granularity ladder. THIS is the single-queue-vs-stealing table.
// -----------------------------------------------------------------------------
int runSweep(nme::JobSystem& js, const Args& a, nme::platform::Timer& timer) {
    // (jobs, work) pairs from many-tiny to few-fat. Total work per row is kept
    // roughly comparable so rows are legible against each other; the left rows
    // are the fine-grained cases where a shared queue convoys and stealing wins.
    struct Row { u32 jobs; u32 work; };
    const Row rows[] = {
        {200'000,     0},   // pathological: trivial jobs, pure scheduler overhead
        {100'000,   100},   // very fine
        { 50'000,   500},   // fine
        { 10'000,  2'000},  // medium
        {  2'000, 10'000},  // coarse
        {    500, 50'000},  // fat (realistic engine granularity)
    };

    std::printf("sweep: frames=%u repeats=%u workers=%u\n", a.frames, a.repeats, js.workerCount());
    std::printf("  %-9s %-8s %-14s %-16s\n", "jobs", "work", "ns/job", "jobs/s");
    std::printf("  ------------------------------------------------------\n");
    for (const Row& row : rows) {
        const f64 ns = medianNsPerJob(js, timer, row.jobs, row.work, a.frames, a.repeats);
        if (ns <= 0.0) {
            std::printf("  %-9u %-8u %-14s %-16s\n", row.jobs, row.work, "(too fast)", "-");
        } else {
            std::printf("  %-9u %-8u %-14.1f %-16.0f\n", row.jobs, row.work, ns, 1e9 / ns);
        }
    }
    std::printf("  sink: %llu\n", static_cast<unsigned long long>(g_sink.load(nme::MemoryOrder::Relaxed)));
    return 0;
}

// -----------------------------------------------------------------------------
// Frame sim -- distinct job types as dependency phases.
// -----------------------------------------------------------------------------
int runFrameSim(nme::JobSystem& js, const Args& a, const nme::platform::Timer& timer) {
    const u32 s = a.scale;
    const u32 kAnim    = 2000 * s;
    const u32 kPhysics =  256 * s;
    const u32 kCulling =  512 * s;
    const u32 kRender  =   64 * s;

    constexpr u32 wAnim    =   800;
    constexpr u32 wPhysics =  6000;
    constexpr u32 wCulling =  1200;
    constexpr u32 wRender  =  4000;

    std::printf("frame-sim: scale=%u frames=%u workers=%u\n", s, a.frames, js.workerCount());
    std::printf("  anim=%u(w%u) physics=%u(w%u) culling=%u(w%u) render=%u(w%u)\n",
                kAnim, wAnim, kPhysics, wPhysics, kCulling, wCulling, kRender, wRender);

    const u64 tStart = nme::platform::Timer::now();
    u64 worstFrameTicks = 0;

    for (u32 f = 0; f < a.frames; ++f) {
        NME_PROFILE_FRAME_MARK();
        const u64 frameStart = nme::platform::Timer::now();

        { nme::JobCounter c; js.runN(kAnim,    [](const u32 i){ sink(busyWork(wAnim,    i)); }, c, "anim.pose");     js.waitForCounter(c); }
        { nme::JobCounter c; js.runN(kPhysics, [](const u32 i){ sink(busyWork(wPhysics, i)); }, c, "physics.solve"); js.waitForCounter(c); }
        { nme::JobCounter c; js.runN(kCulling, [](const u32 i){ sink(busyWork(wCulling, i)); }, c, "cull.frustum");  js.waitForCounter(c); }
        {
            nme::JobCounter c;
            js.runN(kRender, [](const u32 i){ sink(busyWork(wRender, i)); }, c, "render.cmdbuf");
            js.run([]{ sink(busyWork(60000, 7)); }, &c, "render.mainpass");  // straggler
            js.waitForCounter(c);
        }

        const u64 ft = nme::platform::Timer::now() - frameStart;
        if (ft > worstFrameTicks) worstFrameTicks = ft;
    }

    const f64 seconds  = timer.to_seconds(nme::platform::Timer::now() - tStart);
    const f64 avgFrame = (seconds * 1000.0) / static_cast<f64>(a.frames);
    const f64 worstMs  = timer.to_seconds(worstFrameTicks) * 1000.0;

    std::printf("  sink:       %llu\n", static_cast<unsigned long long>(g_sink.load(nme::MemoryOrder::Relaxed)));
    std::printf("  avg frame:  %8.3f ms  (%.1f fps)\n", avgFrame, 1000.0 / avgFrame);
    std::printf("  worst frame:%8.3f ms  <- the straggler's frame\n", worstMs);
    return 0;
}

}  // namespace

int main(const int argc, char** argv) {
    const Args args = parse(argc, argv);

    nme::JobSystem js;
    js.startup();

    nme::platform::Timer& timer = nme::platform::global_timer();
    timer.startup();

    int rc = 0;
    if      (args.verify) rc = runVerify(js);
    else if (args.sweep)  rc = runSweep(js, args, timer);
    else if (args.frame)  rc = runFrameSim(js, args, timer);
    else                  rc = runThroughput(js, args, timer);

    timer.shutdown();
    js.shutdown();
    return rc;
}