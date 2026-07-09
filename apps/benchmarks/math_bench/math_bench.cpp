//==============================================================================
// math_bench -- nme::math vs GLM, head-to-head on the transform hot paths.
//------------------------------------------------------------------------------
//
//   nme_bench_math --benchmark_repetitions=15 --benchmark_report_aggregates_only=true
//   nme_bench_math --benchmark_filter=mat4_mul     # just the matmul pair
//
// Each op registers a /nme and a /glm row over the same K inputs; pair them and
// read the ratio (items_per_second, higher = faster). Keep the comparison
// like-for-like: GLM's SIMD is build-flag dependent (often scalar by default on
// MSVC) -- define GLM_FORCE_INTRINSICS to see GLM's SSE ceiling, and build nme
// with NME_SIMD_SSE for its dot override. Measure matching configs, not one
// library's SIMD path against the other's scalar path.
//==============================================================================

#include <array>
#include <cstdint>

#include <benchmark/benchmark.h>
#include <glm/glm.hpp>

#include "nme/core/math/vec.hpp"
#include "nme/core/math/matrix.hpp"

#if NME_PLATFORM_WINDOWS
#include <DirectXMath.h>   // ships with the Windows SDK; header-only, no link
#endif

namespace {

constexpr int K = 256;  // small, L1-resident: keeps it ALU-bound, not memory-bound

std::uint32_t g_seed = 0x12345u;
inline float frand() {  // cheap deterministic LCG in [-0.5, 0.5)
    g_seed = g_seed * 1664525u + 1013904223u;
    return static_cast<float>(g_seed >> 8) * (1.0f / 16777216.0f) - 0.5f;
}

// One overload per type, tag-dispatched on T* -- no explicit specialization
// (MSVC mishandles those, which is what left frand() looking unused). Brace-init
// evaluates the frand() calls left-to-right, so the fill is deterministic.
inline nme::math::vec4 randOf(nme::math::vec4*) { return {frand(), frand(), frand(), frand()}; }
inline nme::math::vec3 randOf(nme::math::vec3*) { return {frand(), frand(), frand()}; }
inline glm::vec4       randOf(glm::vec4*)       { return {frand(), frand(), frand(), frand()}; }
inline glm::vec3       randOf(glm::vec3*)       { return {frand(), frand(), frand()}; }

inline nme::math::mat4 randOf(nme::math::mat4*) {
    return {frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand(),
            frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand()};
}
inline glm::mat4 randOf(glm::mat4*) {
    return {frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand(),
            frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand()};
}

#if NME_PLATFORM_WINDOWS
inline DirectX::XMFLOAT4 randOf(DirectX::XMFLOAT4*) { return {frand(), frand(), frand(), frand()}; }
inline DirectX::XMFLOAT3 randOf(DirectX::XMFLOAT3*) { return {frand(), frand(), frand()}; }
inline DirectX::XMFLOAT4X4 randOf(DirectX::XMFLOAT4X4*) {
    return {frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand(),
            frand(), frand(), frand(), frand(), frand(), frand(), frand(), frand()};
}
#endif

template<class T>
std::array<T, K> makeArr() {
    std::array<T, K> a{};
    for (auto& e : a) e = randOf(static_cast<T*>(nullptr));
    return a;
}

}  // namespace

// Unqualified dot/cross/normalize and the operators resolve per type via ADL, so
// a single templated body serves both libraries. DoNotOptimize on the array data
// each iteration blocks cross-iteration hoisting; on each result blocks elision.

template<class V>
static void bench_add(benchmark::State& st) {
    auto a = makeArr<V>(); auto b = makeArr<V>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(a.data());
        benchmark::DoNotOptimize(b.data());
        for (int i = 0; i < K; ++i) { V c = a[i] + b[i]; benchmark::DoNotOptimize(c); }
    }
    st.SetItemsProcessed(st.iterations() * K);
}

template<class V>
static void bench_dot(benchmark::State& st) {
    auto a = makeArr<V>(); auto b = makeArr<V>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(a.data());
        benchmark::DoNotOptimize(b.data());
        float acc = 0.0f;
        for (int i = 0; i < K; ++i) acc += dot(a[i], b[i]);
        benchmark::DoNotOptimize(acc);
    }
    st.SetItemsProcessed(st.iterations() * K);
}

template<class V>
static void bench_normalize(benchmark::State& st) {
    auto a = makeArr<V>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(a.data());
        for (int i = 0; i < K; ++i) { V c = normalize(a[i]); benchmark::DoNotOptimize(c); }
    }
    st.SetItemsProcessed(st.iterations() * K);
}

template<class V>
static void bench_cross(benchmark::State& st) {
    auto a = makeArr<V>(); auto b = makeArr<V>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(a.data());
        benchmark::DoNotOptimize(b.data());
        for (int i = 0; i < K; ++i) { V c = cross(a[i], b[i]); benchmark::DoNotOptimize(c); }
    }
    st.SetItemsProcessed(st.iterations() * K);
}

template<class M>
static void bench_matmul(benchmark::State& st) {
    auto a = makeArr<M>(); auto b = makeArr<M>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(a.data());
        benchmark::DoNotOptimize(b.data());
        for (int i = 0; i < K; ++i) { M c = a[i] * b[i]; benchmark::DoNotOptimize(c); }
    }
    st.SetItemsProcessed(st.iterations() * K);
}

template<class M, class V>
static void bench_matvec(benchmark::State& st) {
    auto m = makeArr<M>(); auto v = makeArr<V>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(m.data());
        benchmark::DoNotOptimize(v.data());
        for (int i = 0; i < K; ++i) { V c = m[i] * v[i]; benchmark::DoNotOptimize(c); }
    }
    st.SetItemsProcessed(st.iterations() * K);
}

BENCHMARK(bench_add<nme::math::vec4>)->Name("vec4_add/nme");
BENCHMARK(bench_add<glm::vec4>)->Name("vec4_add/glm");

BENCHMARK(bench_dot<nme::math::vec4>)->Name("vec4_dot/nme");
BENCHMARK(bench_dot<glm::vec4>)->Name("vec4_dot/glm");

BENCHMARK(bench_normalize<nme::math::vec4>)->Name("vec4_normalize/nme");
BENCHMARK(bench_normalize<glm::vec4>)->Name("vec4_normalize/glm");

BENCHMARK(bench_cross<nme::math::vec3>)->Name("vec3_cross/nme");
BENCHMARK(bench_cross<glm::vec3>)->Name("vec3_cross/glm");

BENCHMARK(bench_matmul<nme::math::mat4>)->Name("mat4_mul/nme");
BENCHMARK(bench_matmul<glm::mat4>)->Name("mat4_mul/glm");

BENCHMARK(bench_matvec<nme::math::mat4, nme::math::vec4>)->Name("mat4_vec4/nme");
BENCHMARK(bench_matvec<glm::mat4, glm::vec4>)->Name("mat4_vec4/glm");

#if NME_PLATFORM_WINDOWS
// DirectXMath: load storage type -> XMVECTOR/XMMATRIX -> compute -> store back,
// which is how it's actually used (and keeps the load/store fair vs nme/glm).
namespace dx = DirectX;

static void bench_add_dxm(benchmark::State& st) {
    auto a = makeArr<dx::XMFLOAT4>(); auto b = makeArr<dx::XMFLOAT4>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(a.data());
        benchmark::DoNotOptimize(b.data());
        for (int i = 0; i < K; ++i) {
            dx::XMFLOAT4 c;
            dx::XMStoreFloat4(&c, dx::XMVectorAdd(dx::XMLoadFloat4(&a[i]), dx::XMLoadFloat4(&b[i])));
            benchmark::DoNotOptimize(c);
        }
    }
    st.SetItemsProcessed(st.iterations() * K);
}

static void bench_dot_dxm(benchmark::State& st) {
    auto a = makeArr<dx::XMFLOAT4>(); auto b = makeArr<dx::XMFLOAT4>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(a.data());
        benchmark::DoNotOptimize(b.data());
        float acc = 0.0f;
        for (int i = 0; i < K; ++i)
            acc += dx::XMVectorGetX(dx::XMVector4Dot(dx::XMLoadFloat4(&a[i]), dx::XMLoadFloat4(&b[i])));
        benchmark::DoNotOptimize(acc);
    }
    st.SetItemsProcessed(st.iterations() * K);
}

static void bench_normalize_dxm(benchmark::State& st) {
    auto a = makeArr<dx::XMFLOAT4>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(a.data());
        for (int i = 0; i < K; ++i) {
            dx::XMFLOAT4 c;
            dx::XMStoreFloat4(&c, dx::XMVector4Normalize(dx::XMLoadFloat4(&a[i])));
            benchmark::DoNotOptimize(c);
        }
    }
    st.SetItemsProcessed(st.iterations() * K);
}

static void bench_cross_dxm(benchmark::State& st) {
    auto a = makeArr<dx::XMFLOAT3>(); auto b = makeArr<dx::XMFLOAT3>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(a.data());
        benchmark::DoNotOptimize(b.data());
        for (int i = 0; i < K; ++i) {
            dx::XMFLOAT3 c;
            dx::XMStoreFloat3(&c, dx::XMVector3Cross(dx::XMLoadFloat3(&a[i]), dx::XMLoadFloat3(&b[i])));
            benchmark::DoNotOptimize(c);
        }
    }
    st.SetItemsProcessed(st.iterations() * K);
}

static void bench_matmul_dxm(benchmark::State& st) {
    auto a = makeArr<dx::XMFLOAT4X4>(); auto b = makeArr<dx::XMFLOAT4X4>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(a.data());
        benchmark::DoNotOptimize(b.data());
        for (int i = 0; i < K; ++i) {
            dx::XMFLOAT4X4 c;
            dx::XMStoreFloat4x4(&c, dx::XMMatrixMultiply(dx::XMLoadFloat4x4(&a[i]), dx::XMLoadFloat4x4(&b[i])));
            benchmark::DoNotOptimize(c);
        }
    }
    st.SetItemsProcessed(st.iterations() * K);
}

static void bench_matvec_dxm(benchmark::State& st) {
    auto m = makeArr<dx::XMFLOAT4X4>(); auto v = makeArr<dx::XMFLOAT4>();
    for (auto _ : st) {
        benchmark::DoNotOptimize(m.data());
        benchmark::DoNotOptimize(v.data());
        for (int i = 0; i < K; ++i) {
            dx::XMFLOAT4 c;
            dx::XMStoreFloat4(&c, dx::XMVector4Transform(dx::XMLoadFloat4(&v[i]), dx::XMLoadFloat4x4(&m[i])));
            benchmark::DoNotOptimize(c);
        }
    }
    st.SetItemsProcessed(st.iterations() * K);
}

BENCHMARK(bench_add_dxm)->Name("vec4_add/dxm");
BENCHMARK(bench_dot_dxm)->Name("vec4_dot/dxm");
BENCHMARK(bench_normalize_dxm)->Name("vec4_normalize/dxm");
BENCHMARK(bench_cross_dxm)->Name("vec3_cross/dxm");
BENCHMARK(bench_matmul_dxm)->Name("mat4_mul/dxm");
BENCHMARK(bench_matvec_dxm)->Name("mat4_vec4/dxm");
#endif  // NME_PLATFORM_WINDOWS

BENCHMARK_MAIN();