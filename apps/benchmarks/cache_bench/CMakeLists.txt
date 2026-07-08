add_executable(cache_bench main.cpp)
target_link_libraries(cache_bench PRIVATE nme::engine nme::warnings)
nme_enable_sanitizers(cache_bench)
set_target_properties(cache_bench PROPERTIES FOLDER "benchmark")