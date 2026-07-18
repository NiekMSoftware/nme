include(FetchContent)
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_Declare(benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.4)
FetchContent_MakeAvailable(benchmark)

FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1)
FetchContent_MakeAvailable(glm)

include(add_benchmark)

nme_add_benchmark(cache
        SOURCES cache_bench/aos_vs_soa.cpp)

nme_add_benchmark(jobs
        SOURCES job_bench/job_bench.cpp)

nme_add_benchmark(math
        SOURCES math_bench/math_bench.cpp
        DEPS    glm::glm)

nme_add_benchmark(allocator
        SOURCES allocator_bench/alloc_bench.cpp
)