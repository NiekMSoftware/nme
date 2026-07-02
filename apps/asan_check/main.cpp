// Deliberately triggers a heap-buffer-overflow to verify AddressSanitizer is
// wired up (Ch. 2.4). Select the "x64-ASan" config in VS, or:
//   cmake -S . -B out/asan -DNME_ENABLE_SANITIZERS=ON -DNME_BUILD_ASAN_CHECK=ON
//   cmake --build out/asan
//   ./out/asan/apps/asan_check/asan_check    # ASan aborts with a diagnostic
// Never part of a normal build.
#include <cstddef>
#include <cstdio>

int main() {
    const std::size_t n = 4;
    int *data = new int[n];
    for (std::size_t i = 0; i < n; ++i) data[i] = static_cast<int>(i);

    volatile std::size_t idx = n;              // one past the last valid index
    std::printf("value = %d\n", data[idx]);    // <-- heap-buffer-overflow

    delete[] data;
    return 0;
}
