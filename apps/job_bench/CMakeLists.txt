add_executable(job_bench main.cpp)
target_link_libraries(job_bench PRIVATE nme::engine nme::warnings)
nme_enable_sanitizers(job_bench)
set_target_properties(job_bench PROPERTIES FOLDER "benchmark")