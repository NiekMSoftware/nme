//==============================================================================
// alloc_bench -- custom allocators vs OS heap (Google Benchmark)
//------------------------------------------------------------------------------
// Wire up with:  nme_add_benchmark(allocator SOURCES allocator_bench/alloc_bench.cpp)
//
// Backing blocks are allocated once per case (before the timed loop), so the
// custom allocators never pay OS-alloc cost inside a measurement. malloc/new
// pay it every op -- that IS the thing under test.
//
//   ./nme_bench_allocator                                     # all cases
//   ./nme_bench_allocator --benchmark_filter=Tight            # the headline: 1 op/iter
//   ./nme_bench_allocator --benchmark_filter=Batch            # frame-shaped: N then release
//   ./nme_bench_allocator --benchmark_repetitions=5 --benchmark_report_aggregates_only=true
//   ./nme_bench_allocator --verify                            # correctness, then exit
//
// The 4e deliverable ("beats malloc on a tight loop") is the Tight group: read
// Time (ns/op) for Tight_Stack / Tight_Pool against Tight_Malloc / Tight_New at
// each size. Bump + rewind and free-list pop/push are a handful of instructions
// with no kernel transition; the OS heap is not. Batch shows the same win from
// the other angle: the custom side bulk-releases (clear/swap) instead of freeing
// N times.
//==============================================================================
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

#include <benchmark/benchmark.h>

#include "nme/core/debug/profiler.h"
#include "nme/core/memory/pool_alloc.h"
#include "nme/core/memory/scratch_alloc.h"
#include "nme/core/memory/stack_alloc.h"
#include "nme/platform/types.h"

//------------------------------------------------------------------------------
// Targets the canonical core/memory headers: stack_alloc.h, pool_alloc.h,
// scratch_alloc.h. Direct calls only (no vtbl) -- this measures the raw ceiling.
//------------------------------------------------------------------------------

namespace {

using nme::u32;
using nme::u64;
using nme::u8;
using nme::usize;
using nme::uptr;

constexpr usize kAlign = 16;

// Stride a raw size occupies once aligned -- what the stack consumes per alloc
// and the pool's block size. Keeps backing-size math honest.
inline usize stride_of(const usize sz) { return (sz + (kAlign - 1)) & ~(kAlign - 1); }

// Aligned OS backing block for the custom allocators. Not timed. Over-aligned to
// 64 (cache line) so the pool's alignment assert is always satisfied.
u8* backing_alloc(const usize bytes) {
    const usize rounded = (bytes + 63) / 64 * 64;
#if defined(_WIN32)
    return static_cast<u8*>(_aligned_malloc(rounded, 64));
#else
    return static_cast<u8*>(std::aligned_alloc(64, rounded));
#endif
}
void backing_free(void* p) {
#if defined(_WIN32)
    _aligned_free(p);
#else
    std::free(p);
#endif
}

// Un-elidable pointer sink. DoNotOptimize forces the allocation to actually
// happen; without it the optimizer deletes the whole loop.
inline void touch(void* p) { benchmark::DoNotOptimize(p); }

}  // namespace

//==============================================================================
// TIGHT LOOP -- exactly one alloc + one release per iteration. Time = ns/op.
// This is the direct "do we beat the OS heap" comparison.
//==============================================================================

// --- OS baselines ---
void BM_Tight_Malloc(benchmark::State& state) {
    const auto sz = static_cast<usize>(state.range(0));
    for (auto _ : state) {
        void* p = std::malloc(sz);
        touch(p);
        std::free(p);
    }
    state.SetItemsProcessed(state.iterations());
}

void BM_Tight_New(benchmark::State& state) {
    const auto sz = static_cast<usize>(state.range(0));
    for (auto _ : state) {
        void* p = ::operator new(sz);
        touch(p);
        ::operator delete(p, sz);  // sized delete: fair match to malloc/free
    }
    state.SetItemsProcessed(state.iterations());
}

// --- stack: bump up, roll back to a marker (LIFO of one) ---
void BM_Tight_Stack(benchmark::State& state) {
    const auto sz = static_cast<usize>(state.range(0));
    u8* mem = backing_alloc(stride_of(sz) + kAlign);
    nme::StackAllocator a{};
    nme::stack_alloc_init(&a, mem, stride_of(sz) + kAlign);
    const nme::StackMarker base = nme::stack_alloc_get_marker(&a);
    for (auto _ : state) {
        void* p = nme::stack_alloc(&a, sz, kAlign);
        touch(p);
        nme::stack_alloc_free_to_marker(&a, base);
    }
    state.SetItemsProcessed(state.iterations());
    backing_free(mem);
}

// --- pool: pop then push the same fixed-size block ---
void BM_Tight_Pool(benchmark::State& state) {
    const auto sz = static_cast<usize>(state.range(0));
    constexpr usize kBlocks = 256;
    u8* mem = backing_alloc(stride_of(sz) * kBlocks);
    nme::PoolAllocator p{};
    nme::pool_alloc_init(&p, mem, sz, kBlocks, kAlign);
    for (auto _ : state) {
        void* b = nme::pool_alloc(&p);
        touch(b);
        nme::pool_free(&p, b);
    }
    state.SetItemsProcessed(state.iterations());
    backing_free(mem);
}

// Same size sweep across all four so the rows line up in compare.py.
#define NME_TIGHT_SIZES ->Arg(16)->Arg(64)->Arg(256)->Arg(1024)->ArgName("bytes")->Unit(benchmark::kNanosecond)
BENCHMARK(BM_Tight_Malloc) NME_TIGHT_SIZES;
BENCHMARK(BM_Tight_New)    NME_TIGHT_SIZES;
BENCHMARK(BM_Tight_Stack)  NME_TIGHT_SIZES;
BENCHMARK(BM_Tight_Pool)   NME_TIGHT_SIZES;
#undef NME_TIGHT_SIZES

//==============================================================================
// BATCH / FRAME -- allocate N blocks, then release the whole batch. This is how
// these allocators are really used (a frame's worth of scratch, a level's data).
// The custom side releases in O(1) (clear/swap) or O(N) pushes (pool); the OS
// side must free N times. Args: {N, bytes}. Time is per batch; items/s = allocs/s.
//==============================================================================
namespace {
// One free-order-preserving pointer table, reused by the OS baselines so their
// bookkeeping (which the custom allocators don't need) isn't counted as per-op.
std::vector<void*> g_ptrs;
}  // namespace

void BM_Batch_Malloc(benchmark::State& state) {
    const u32   n  = static_cast<u32>(state.range(0));
    const auto sz = static_cast<usize>(state.range(1));
    g_ptrs.resize(n);
    for (auto _ : state) {
        NME_PROFILE_FRAME_MARK();
        for (u32 i = 0; i < n; ++i) { g_ptrs[i] = std::malloc(sz); touch(g_ptrs[i]); }
        for (u32 i = 0; i < n; ++i) std::free(g_ptrs[i]);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}

void BM_Batch_Stack(benchmark::State& state) {
    const u32   n   = static_cast<u32>(state.range(0));
    const auto sz  = static_cast<usize>(state.range(1));
    const usize cap = stride_of(sz) * n + kAlign;
    u8* mem = backing_alloc(cap);
    nme::StackAllocator a{};
    nme::stack_alloc_init(&a, mem, cap);
    for (auto _ : state) {
        NME_PROFILE_FRAME_MARK();
        for (u32 i = 0; i < n; ++i) { void* p = nme::stack_alloc(&a, sz, kAlign); touch(p); }
        nme::stack_alloc_clear(&a);  // release the whole frame in one store
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
    backing_free(mem);
}

void BM_Batch_Pool(benchmark::State& state) {
    const u32   n  = static_cast<u32>(state.range(0));
    const auto sz = static_cast<usize>(state.range(1));
    u8* mem = backing_alloc(stride_of(sz) * n);
    nme::PoolAllocator p{};
    nme::pool_alloc_init(&p, mem, sz, n, kAlign);
    g_ptrs.resize(n);
    for (auto _ : state) {
        NME_PROFILE_FRAME_MARK();
        for (u32 i = 0; i < n; ++i) { g_ptrs[i] = nme::pool_alloc(&p); touch(g_ptrs[i]); }
        for (u32 i = 0; i < n; ++i) nme::pool_free(&p, g_ptrs[i]);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
    backing_free(mem);
}

void BM_Batch_Scratch(benchmark::State& state) {
    const u32   n   = static_cast<u32>(state.range(0));
    const auto sz  = static_cast<usize>(state.range(1));
    const usize cap = stride_of(sz) * n + kAlign;
    u8* a = backing_alloc(cap);
    u8* b = backing_alloc(cap);
    nme::ScratchAllocator s{};
    nme::scratch_alloc_init(&s, a, b, cap);
    for (auto _ : state) {
        NME_PROFILE_FRAME_MARK();
        for (u32 i = 0; i < n; ++i) { void* p = nme::scratch_alloc(&s, sz, kAlign); touch(p); }
        nme::scratch_alloc_swap(&s);  // flip buffers; last frame's data survives
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
    backing_free(a);
    backing_free(b);
}

#define NME_BATCH_ARGS \
    ->Args({1000, 64})->Args({10000, 64})->Args({10000, 256}) \
    ->ArgNames({"N", "bytes"})->Unit(benchmark::kMicrosecond)
BENCHMARK(BM_Batch_Malloc)  NME_BATCH_ARGS;
BENCHMARK(BM_Batch_Stack)   NME_BATCH_ARGS;
BENCHMARK(BM_Batch_Pool)    NME_BATCH_ARGS;
BENCHMARK(BM_Batch_Scratch) NME_BATCH_ARGS;
#undef NME_BATCH_ARGS

//==============================================================================
// Correctness -- not a benchmark. --verify flag keeps this a drop-in; the real
// home for these asserts is the memory test target.
//==============================================================================
namespace {
int runVerify() {
    using namespace nme;

    // stack: rewind to a marker reuses the exact bytes above it.
    {
        u8* mem = backing_alloc(4096);
        StackAllocator a{}; stack_alloc_init(&a, mem, 4096);
        void* p0 = stack_alloc(&a, 100, kAlign);
        StackMarker m = stack_alloc_get_marker(&a);
        void* p1 = stack_alloc(&a, 200, kAlign);
        stack_alloc_free_to_marker(&a, m);
        void* p2 = stack_alloc(&a, 8, kAlign);
        assert(p2 == p1 && "rewind must hand back the rewound address");
        assert((reinterpret_cast<uptr>(p0) % kAlign) == 0 && "alignment");
        backing_free(mem);
    }
    // pool: a freed block is the next one handed out.
    {
        constexpr usize sz = 32;
        u8* mem = backing_alloc(stride_of(sz) * 8);
        PoolAllocator p{}; pool_alloc_init(&p, mem, sz, 8, kAlign);
        void* x = pool_alloc(&p);
        void* y = pool_alloc(&p);
        pool_free(&p, x);
        void* z = pool_alloc(&p);
        assert(z == x && y != x && "freed block must be recycled");
        backing_free(mem);
    }
    // scratch: swap moves you to the other buffer.
    {
        constexpr usize cap = 1024;
        u8* a = backing_alloc(cap);
        u8* b = backing_alloc(cap);
        ScratchAllocator s{}; scratch_alloc_init(&s, a, b, cap);
        void* r0 = scratch_alloc(&s, 64, kAlign);
        scratch_alloc_swap(&s);
        void* r1 = scratch_alloc(&s, 64, kAlign);
        assert(r0 != r1 && "swap must switch buffers");
        backing_free(a);
        backing_free(b);
    }
    std::printf("PASS\n");
    return 0;
}
}  // namespace

int main(int argc, char** argv) {
    bool verify = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--verify") == 0) verify = true;
    }
    if (verify) return runVerify();

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}