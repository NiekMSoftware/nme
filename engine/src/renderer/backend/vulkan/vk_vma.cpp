// Compiles the Vulkan Memory Allocator implementation. EXACTLY ONE .cpp in the
// whole build may define VMA_IMPLEMENTATION -- this is that file. Everywhere else
// just includes <vk_mem_alloc.h> for the declarations.
#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION

// VMA's implementation is third-party and won't compile clean under the engine's
// warnings-as-errors. Silence warnings for this TU only; our own code stays strict.
#if defined(_MSC_VER)
    #pragma warning(push, 0)
#elif defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wall"
    #pragma GCC diagnostic ignored "-Wextra"
    #pragma GCC diagnostic ignored "-Wpedantic"
    #pragma GCC diagnostic ignored "-Wunused-parameter"
    #pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include <vk_mem_alloc.h>

#if defined(_MSC_VER)
    #pragma warning(pop)
#elif defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif
