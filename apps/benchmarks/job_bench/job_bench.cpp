//==============================================================================
// job_bench -- job-system throughput / scaling / frame-sim (Google Benchmark)
//------------------------------------------------------------------------------
// Wire up with:  nme_add_benchmark(jobs SOURCES job_bench.cpp)
//
// One JobSystem + Timer are started once in main and shared by every case, so
// pool start-up never pollutes a measurement.
//
//   ./nme_bench_jobs                                          # all cases
//   ./nme_bench_jobs --benchmark_filter=Throughput
//   ./nme_bench_jobs --benchmark_filter=Grain                # grain sweep: recursive vs flat
//   ./nme_bench_jobs --benchmark_repetitions=5 \
//                    --benchmark_report_aggregates_only=true  # median/stddev/cv
//   ./nme_bench_jobs --verify                                 # correctness, then exit
//
// The granularity sweep is the 4e deliverable: build once work-stealing and once
// single-queue, run --benchmark_filter=Throughput on each, diff with
// tools/compare.py. The small-job rows (jobs >> work) are where stealing wins.
//==============================================================================
#include <algorithm>  // std::max_element
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

#include <benchmark/benchmark.h>

#include "nme/core/debug/profiler.h"
#include "nme/core/jobs/job_system.h"
#include "nme/platform/thread/atomics.h"
#include "nme/platform/timer/timer.h"
#include "nme/platform/types.h"

namespace {

using nme::u32;
using nme::u64;

nme::JobSystem* g_js = nullptr;  // owned by main, shared by all benchmarks

// A tiny, un-optimizable-away compute kernel. `work` scales per-job cost.
u64 busyWork(const u32 work, const u32 seed) {
    u64 acc = seed;
    for (u32 i = 0; i < work; ++i) {
        acc = acc * 6364136223846793005ULL + 1442695040888963407ULL;  // LCG step
        acc ^= acc >> 17;
    }
    return acc;
}

// Per-thread sink: keeps the compiler from eliding the work without making 15
// workers ping-pong one cache line (that contention was inflating the ~500 ns/job
// floor). thread_local has static storage duration so the writes stay live; main
// anchors it once via DoNotOptimize so it can't be proven dead.
thread_local u64 t_sink = 0;
inline void sink(const u64 v) { t_sink += v; }

}  // namespace

//------------------------------------------------------------------------------
// Throughput / granularity sweep. Args: {jobs, work}, one instance per row.
// Time is per frame (dispatch + wait); items/s is jobs/s; ns/job = Time / jobs.
//------------------------------------------------------------------------------
void BM_JobThroughput(benchmark::State& state) {
    const u32 jobs = static_cast<u32>(state.range(0));
    const u32 work = static_cast<u32>(state.range(1));

    for (auto _ : state) {
        NME_PROFILE_FRAME_MARK();
        nme::JobCounter counter;
        // recursive-split dispatch; swap to runN() to A/B against flat chunking
        g_js->parallelFor(jobs, [work](const u32 i) { sink(busyWork(work, i)); }, counter, "bench.work");
        g_js->waitForCounter(counter);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(jobs));  // -> jobs/s
    state.counters["workers"] = static_cast<double>(g_js->workerCount());
}
BENCHMARK(BM_JobThroughput)
    ->Args({200'000,      0})   // pathological: trivial jobs, pure scheduler overhead
    ->Args({100'000,    100})   // very fine
    ->Args({ 50'000,    500})   // fine
    ->Args({ 10'000,  2'000})   // medium
    ->Args({  2'000, 10'000})   // coarse
    ->Args({    500, 50'000})   // fat (realistic engine granularity)
    ->ArgNames({"jobs", "work"})
    ->Unit(benchmark::kMicrosecond)
    ->MeasureProcessCPUTime()  // CPU column = all workers, not just main
    ->UseRealTime();           // Time column (and items/s) = wall clock

//------------------------------------------------------------------------------
// Grain sweep -- fixed workload, vary grain, one row per grain, for both dispatch
// strategies. Chart median/cv vs grain: recursive (parallelFor) should hold flat
// where the flat chunker (runN) climbs from submitter-side dispatch pressure. The
// crossover at the fine end is where per-job overhead overtakes either way.
//   --benchmark_filter=Grain --benchmark_repetitions=15 --benchmark_report_aggregates_only=true
//------------------------------------------------------------------------------
namespace {
constexpr u32 kSweepJobs = 100'000;  // fixed count; grain (items/leaf) is the axis
constexpr u32 kSweepWork = 500;
}  // namespace

void BM_GrainRecursive(benchmark::State& state) {
    const u32 grain = static_cast<u32>(state.range(0));
    for (auto _ : state) {
        NME_PROFILE_FRAME_MARK();
        nme::JobCounter c;
        g_js->parallelFor(kSweepJobs, [](u32 i) { sink(busyWork(kSweepWork, i)); }, c, "bench.work", grain);
        g_js->waitForCounter(c);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSweepJobs));
}

void BM_GrainFlat(benchmark::State& state) {
    const u32 grain = static_cast<u32>(state.range(0));
    for (auto _ : state) {
        NME_PROFILE_FRAME_MARK();
        nme::JobCounter c;
        g_js->runN(kSweepJobs, [](u32 i) { sink(busyWork(kSweepWork, i)); }, c, "bench.work", grain);
        g_js->waitForCounter(c);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSweepJobs));
}

// coarse -> fine: chunk/leaf count = kSweepJobs / grain = {24, 98, 391, 1563, 6250}
static void GrainArgs(benchmark::internal::Benchmark* b) {
    for (int g : {4096, 1024, 256, 64, 16}) b->Arg(g);
    b->ArgName("grain")->Unit(benchmark::kMicrosecond)->MeasureProcessCPUTime()->UseRealTime();
}
BENCHMARK(BM_GrainRecursive)->Apply(GrainArgs);
BENCHMARK(BM_GrainFlat)->Apply(GrainArgs);

//------------------------------------------------------------------------------
// Frame sim -- distinct job types as dependency phases (anim -> physics ->
// culling -> render), one fat straggler. One iteration = one frame, timed by hand
// so Time is the true mean frame and worst_ms is the real worst SINGLE frame (not
// an across-repetition aggregate). CPU/Time = cores busy; the gap is barrier idle.
//------------------------------------------------------------------------------
void BM_FrameSim(benchmark::State& state) {
    const u32 s = static_cast<u32>(state.range(0));
    const u32 kAnim = 2000 * s, kPhysics = 256 * s, kCulling = 512 * s, kRender = 64 * s;
    constexpr u32 wAnim = 800, wPhysics = 6000, wCulling = 1200, wRender = 4000;

    auto& timer = nme::platform::global_timer();
    double worstMs = 0.0, sumMs = 0.0;
    for (auto _ : state) {
        NME_PROFILE_FRAME_MARK();
        const u64 t0 = nme::platform::Timer::now();

        { nme::JobCounter c; g_js->parallelFor(kAnim,    [](u32 i){ sink(busyWork(wAnim,    i)); }, c, "anim.pose");     g_js->waitForCounter(c); }
        { nme::JobCounter c; g_js->parallelFor(kPhysics, [](u32 i){ sink(busyWork(wPhysics, i)); }, c, "physics.solve"); g_js->waitForCounter(c); }
        { nme::JobCounter c; g_js->parallelFor(kCulling, [](u32 i){ sink(busyWork(wCulling, i)); }, c, "cull.frustum");  g_js->waitForCounter(c); }
        {
            nme::JobCounter c;
            g_js->parallelFor(kRender, [](u32 i){ sink(busyWork(wRender, i)); }, c, "render.cmdbuf");
            g_js->run([]{ sink(busyWork(60000, 7)); }, &c, "render.mainpass");  // fat straggler: one job, cannot split
            g_js->waitForCounter(c);
        }

        const double frameSec = timer.to_seconds(nme::platform::Timer::now() - t0);
        state.SetIterationTime(frameSec);                 // Time column = true per-frame wall
        const double frameMs = frameSec * 1000.0;
        sumMs += frameMs;
        if (frameMs > worstMs) worstMs = frameMs;
    }
    const double meanMs = sumMs / static_cast<double>(state.iterations());
    state.counters["worst_ms"]   = worstMs;                            // real worst single frame in the run
    state.counters["worst/mean"] = meanMs > 0.0 ? worstMs / meanMs : 0.0;  // tail spread; 1.0 = perfectly flat
    state.counters["workers"]    = static_cast<double>(g_js->workerCount());
}
BENCHMARK(BM_FrameSim)
    ->Arg(1)->Arg(4)->Arg(16)
    ->ArgName("scale")
    ->Unit(benchmark::kMillisecond)
    ->UseManualTime()          // Time = my per-frame measurement, not GBench's loop timer
    ->MeasureProcessCPUTime();

//------------------------------------------------------------------------------
// Correctness -- not a benchmark. Kept as a --verify flag so the file stays a
// drop-in replacement, but this really belongs in the test target.
//------------------------------------------------------------------------------
namespace {
int runVerify(nme::JobSystem& js) {
    using namespace nme;
    constexpr u32 kJobCount = 10'000;

    Atomic<u32> ran{0};
    { JobCounter c; js.runN(kJobCount, [&ran](u32) { ran.fetchAdd(1, MemoryOrder::Relaxed); }, c); js.waitForCounter(c); }
    const u32 total = ran.load(MemoryOrder::Acquire);
    std::printf("ran: %u / %u\n", total, kJobCount);
    assert(total == kJobCount && "every job must run exactly once");

    static u32 slots[kJobCount] = {};
    { JobCounter c; js.runN(kJobCount, [](const u32 i) { slots[i] = i * 2 + 1; }, c); js.waitForCounter(c); }
    for (u32 i = 0; i < kJobCount; ++i) {
        assert(slots[i] == i * 2 + 1 && "each slot must be written by its job");
    }
    std::printf("PASS\n");
    return 0;
}
}  // namespace

int main(int argc, char** argv) {
    nme::JobSystem js;
    js.startup();
    nme::platform::global_timer().startup();

    bool verify = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--verify") == 0) verify = true;
    }

    int rc = 0;
    if (verify) {
        rc = runVerify(js);
    } else {
        g_js = &js;
        benchmark::Initialize(&argc, argv);
        if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
            rc = 1;
        } else {
            benchmark::RunSpecifiedBenchmarks();
            benchmark::Shutdown();
        }
        g_js = nullptr;
    }

    nme::platform::global_timer().shutdown();
    js.shutdown();
    benchmark::DoNotOptimize(t_sink);  // anchor the per-thread sink
    return rc;
}