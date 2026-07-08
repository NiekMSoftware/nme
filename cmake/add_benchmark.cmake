# cmake/nme_add_benchmark.cmake
#
# nme_add_benchmark(<name>
#     SOURCES <src>...
#     [DEPS   <target>...])
#
# Builds a Google Benchmark executable, links it against the engine, and files
# it under the "benchmarks" IDE folder. Skipped unless NME_BUILD_BENCHMARKS=ON.

function(nme_add_benchmark name)
    if(NOT NME_BUILD_BENCHMARKS)
        return()
    endif()

    cmake_parse_arguments(arg "" "" "SOURCES;DEPS" ${ARGN})
    if(NOT arg_SOURCES)
        message(FATAL_ERROR "nme_add_benchmark(${name}): SOURCES is required")
    endif()

    set(target "nme_bench_${name}")
    add_executable(${target} ${arg_SOURCES})
    target_link_libraries(${target}
            PRIVATE nme::engine benchmark::benchmark_main ${arg_DEPS})
    target_compile_features(${target} PRIVATE cxx_std_20)

    # The line you asked for: every benchmark lands in one IDE folder.
    set_target_properties(${target} PROPERTIES FOLDER "benchmarks")
endfunction()