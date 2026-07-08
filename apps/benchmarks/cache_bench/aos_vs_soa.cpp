// aos_vs_soa.cpp -- AoS vs SoA hot update + cache-line stride probe (Google Benchmark)
//
// Wire up with:  nme_add_benchmark(memory_layout SOURCES aos_vs_soa.cpp)
// Run (Release):
//   ./nme_bench_memory_layout --benchmark_counters_tabular=true
//   ./nme_bench_memory_layout --benchmark_repetitions=8 --benchmark_report_aggregates_only=true
// For the SoA SIMD ceiling, configure the Release preset with -O3 -march=native.
//
// The framework owns timing now. The old best-of-TRIALS loop, the argc-fed `g`,
// and the volatile sink are gone: DoNotOptimize/ClobberMemory keep the reads and
// writes observable so the optimizer can't fuse iterations (pos += k*g) or hoist
// the strided sum out of the state loop.

#include <cstdint>
#include <vector>

#include <benchmark/benchmark.h>

using u8  = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using f32 = float;

struct Vec3 {
    f32 x, y, z;
};

// A realistically fat entity: exactly one 64B line. The hot loop needs only the
// 12B position; in AoS the other 52B ride through cache whether we want them or not.
struct EntityAoS {
    Vec3 pos;        // 12  <- hot
    Vec3 vel;        // 12
    f32  orient[4];  // 16
    f32  mass, drag; //  8
    u32  flags, id;  //  8
    u32  pad[2];     //  8
};
static_assert(sizeof(EntityAoS) == 64, "want one cache line per entity");

namespace {
// AoS 384MB, SoA hot 72MB : both well past a ~33MB L3, so we measure DRAM traffic.
constexpr size_t kEntities = 6'000'000;
}  // namespace

//------------------------------------------------------------------------------
// Position-only update. Compare the two rows' Time (or run tools/compare.py):
// the SoA/AoS ratio should approach the 64/12 = 5.3x line-traffic ceiling.
//------------------------------------------------------------------------------
void BM_LayoutAoS(benchmark::State& state) {
    std::vector<EntityAoS> aos(kEntities, EntityAoS{});
    f32 g = 0.0009765625f;
    benchmark::DoNotOptimize(g);  // opaque to the optimizer

    for (auto _ : state) {
        for (auto& e : aos) {
            e.pos.x += g;
            e.pos.y += g;
            e.pos.z += g;
        }                            // stride 64B, uses 12
        benchmark::ClobberMemory();  // writes observable -> no cross-iteration fusion
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kEntities));
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(kEntities) * sizeof(EntityAoS));  // line traffic
}
BENCHMARK(BM_LayoutAoS)->Unit(benchmark::kMillisecond);

void BM_LayoutSoA(benchmark::State& state) {
    std::vector<f32> px(kEntities, 0.f), py(kEntities, 0.f), pz(kEntities, 0.f);
    f32 g = 0.0009765625f;
    benchmark::DoNotOptimize(g);

    for (auto _ : state) {
        for (size_t i = 0; i < kEntities; ++i) {
            px[i] += g;
            py[i] += g;
            pz[i] += g;
        }                            // three packed streams
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kEntities));
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(kEntities) * 3 * sizeof(f32));  // useful only
}
BENCHMARK(BM_LayoutSoA)->Unit(benchmark::kMillisecond);

//------------------------------------------------------------------------------
// Cache-line stride probe. One benchmark instance per stride (state.range(0)):
// items/s flattens once each access owns a fetch unit -- the plateau onset is the
// transfer granularity. On x86 the adjacent-line prefetcher pairs lines, so the
// knee lands near 128B though the architectural line is 64B
// (getconf LEVEL1_DCACHE_LINESIZE).
//------------------------------------------------------------------------------
void BM_Stride(benchmark::State& state) {
    const size_t stride = static_cast<size_t>(state.range(0));
    const size_t bytes  = 256ull << 20;  // 256MB >> L3 : every stride misses to DRAM
    std::vector<u8> buf(bytes, u8{1});

    for (auto _ : state) {
        u64 acc = 0;
        benchmark::DoNotOptimize(buf.data());  // buf may have changed -> re-read, no hoist
        for (size_t i = 0; i < bytes; i += stride) acc += buf[i];
        benchmark::DoNotOptimize(acc);
    }
    const int64_t accesses = static_cast<int64_t>((bytes + stride - 1) / stride);
    state.SetItemsProcessed(state.iterations() * accesses);  // ns/access = 1e9 / items_per_second
}

static void StrideArgs(benchmark::internal::Benchmark* b) {
    for (int s : {4, 8, 16, 32, 48, 64, 96, 128, 192, 256, 512, 1024}) b->Arg(s);
}
BENCHMARK(BM_Stride)->Apply(StrideArgs)->ArgName("stride")->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();