#ifndef NME_PLATFORM_DEBUG_ASSERT_H_
#define NME_PLATFORM_DEBUG_ASSERT_H_

#include "nme/platform/debug/debug_break.h"

#if NME_DEBUG
    #define NME_PLATFORM_ASSERT(expr) \
        do {                          \
            if (expr) {               \
            } else {                  \
                NME_DEBUG_BREAK();    \
            }                         \
        } while (0)
#else
    #define NME_PLATFORM_ASSERT(expr) ((void)0)
#endif

#endif