// main.cpp -- AoS vs SoA hot update + cache-line stride probe
//
// Build (Release):
//   g++ -O2 -std=c++17 main.cpp -o main
//   g++ -O3 -march=native -std=c++17 main.cpp -o main   # SoA SIMD ceiling
//
// Standalone on <chrono> so it runs anywhere; swap in nme::platform::global_timer()
// / NME_PROFILE_SCOPE if you want it reporting through the engine clock.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using f32 = float;
using f64 = double;
using Clock = std::chrono::steady_clock;
static f64 ns(Clock::duration d) {
    return std::chrono::duration<f64, std::nano>(d).count();
}

//------------------------------------------------------------------------------
// A realistically fat entity: exactly one 64B line. The hot loop needs only the
// 12B position; in AoS the other 52B ride through cache whether we want them or not.
//------------------------------------------------------------------------------
struct Vec3 {
    f32 x, y, z;
};
struct EntityAoS {
    Vec3 pos;        // 12  <- hot
    Vec3 vel;        // 12
    f32 orient[4];   // 16
    f32 mass, drag;  //  8
    u32 flags, id;   //  8
    u32 pad[2];      //  8
};
static_assert(sizeof(EntityAoS) == 64, "want one cache line per entity");

// Measurement note: do NOT wrap the update in an inner "for each frame" loop and
// divide by the frame count -- the optimizer fuses R identical passes into one
// (pos += R*g) and the division then reports a time ~R x too low. Instead: one
// real pass per trial, keep the best, and read a live sample of the array each
// trial so the passes stay observable and cannot be collapsed.

static void bench_layout(int argc) {
    const size_t N = 6'000'000;  // AoS 384MB, SoA hot 72MB : both past a 33MB L3
    const int TRIALS = 8;
    const f32 g = 0.0009765625f * (f32)argc;  // runtime -> not a compile-time constant

    std::vector<EntityAoS> aos(N);
    std::vector<f32> px(N), py(N), pz(N), vx(N), vy(N), vz(N), ox(N), oy(N), oz(N), ow(N), mass(N),
        drag(N);
    std::vector<u32> flags(N), id(N);
    for (size_t i = 0; i < N; i++) {
        aos[i] = EntityAoS{};
        px[i] = py[i] = pz[i] = 0.f;
    }

    volatile f64 sink = 0;

    f64 aos_best = 1e300;
    for (int t = 0; t < TRIALS; t++) {
        auto a = Clock::now();
        for (auto& e : aos) {
            e.pos.x += g;
            e.pos.y += g;
            e.pos.z += g;
        }  // stride 64B, use 12
        auto b = Clock::now();
        aos_best = std::min(aos_best, ns(b - a));
        sink += aos[(size_t)t * 7919 % N].pos.x;
    }
    f64 soa_best = 1e300;
    for (int t = 0; t < TRIALS; t++) {
        auto a = Clock::now();
        for (size_t i = 0; i < N; i++) {
            px[i] += g;
            py[i] += g;
            pz[i] += g;
        }  // packed streams
        auto b = Clock::now();
        soa_best = std::min(soa_best, ns(b - a));
        sink += px[(size_t)t * 7919 % N];
    }

    const f64 aos_pe = aos_best / (f64)N, soa_pe = soa_best / (f64)N;
    std::printf("AoS vs SoA  position-only update  (N=%zu, best of %d)\n", N, TRIALS);
    std::printf("  AoS  %6.3f ns/entity   ~%.1f GB/s of 64B lines\n", aos_pe,
                (f64)N * 64.0 / aos_best);
    std::printf("  SoA  %6.3f ns/entity   ~%.1f GB/s of useful 12B\n", soa_pe,
                (f64)N * 12.0 / soa_best);
    std::printf("  SoA speedup: %.2fx   (ceiling ~64/12 = 5.3x of line traffic)\n\n",
                aos_pe / soa_pe);
    std::printf("  [sink %g]\n\n", (double)sink);
}

static void bench_stride(int argc) {
    const size_t B = 256ull << 20;  // 256MB >> L3 : every stride misses to DRAM
    const int TRIALS = 6;
    std::vector<u8> buf(B, (u8)argc);
    volatile u64 sink = 0;

    const size_t strides[] = {4, 8, 16, 32, 48, 64, 96, 128, 192, 256, 512, 1024};
    std::printf("Cache-line stride probe  (256MB buffer, best of %d)\n", TRIALS);
    std::printf("  stride(B)   ns/access\n");
    for (size_t S : strides) {
        f64 best = 1e300;
        for (int t = 0; t < TRIALS; t++) {
            u64 acc = 0;
            auto a = Clock::now();
            for (size_t i = 0; i < B; i += S) acc += buf[i];
            auto b = Clock::now();
            best = std::min(best, ns(b - a));
            sink += acc;
        }
        std::printf("  %6zu      %7.3f\n", S, best / ((f64)B / (f64)S));
    }
    std::printf("  [sink %llu]\n", (unsigned long long)sink);
    std::printf(
        "  ns/access rises with stride until each access owns a fetch unit, then\n"
        "  flattens; the plateau onset is the transfer granularity. On x86 the\n"
        "  adjacent-line prefetcher pairs lines, so the knee lands near 128B even\n"
        "  though the architectural line is 64B (getconf LEVEL1_DCACHE_LINESIZE).\n");
}

int main(int argc, char**) {
    bench_layout(argc);
    bench_stride(argc);
    return 0;
}