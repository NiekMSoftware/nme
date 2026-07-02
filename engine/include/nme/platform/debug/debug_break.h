#ifndef NME_PLATFORM_DEBUG_DEBUG_BREAK_H_
#define NME_PLATFORM_DEBUG_DEBUG_BREAK_H_

#if defined(_MSC_VER)
    #define NME_DEBUG_BREAK() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
    #define NME_DEBUG_BREAK() __builtin_trap()
#else
    #include <csignal>
    #define NME_DEBUG_BREAK() raise(SIGTRAP)
#endif

#endif