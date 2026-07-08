include(FetchContent)
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_Declare(benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.4)
FetchContent_MakeAvailable(benchmark)

include(add_benchmark)

nme_add_benchmark(cache
        SOURCES cache_bench/aos_vs_soa.cpp)

nme_add_benchmark(jobs
        SOURCES job_bench/job_bench.cpp)