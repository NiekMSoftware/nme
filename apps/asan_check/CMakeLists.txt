# Deliberately-buggy program to confirm the sanitizer build works (Ch. 2.4).
# Built only with -DNME_BUILD_ASAN_CHECK=ON; pair with -DNME_ENABLE_SANITIZERS=ON
# (or select the "x64-ASan" configuration in Visual Studio).
add_executable(asan_check main.cpp)
target_link_libraries(asan_check PRIVATE nme::warnings)
nme_enable_sanitizers(asan_check)
set_target_properties(asan_check PROPERTIES FOLDER "check")