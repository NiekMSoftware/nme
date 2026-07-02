add_executable(sandbox main.cpp)
target_link_libraries(sandbox PRIVATE nme::engine nme::warnings)
nme_enable_sanitizers(sandbox)
