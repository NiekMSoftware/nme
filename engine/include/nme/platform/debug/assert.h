#pragma once

#include "nme/platform/platform.h"
#include "nme/platform/debug/debug_break.h"
#include "nme/platform/types.h"

namespace nme::detail
{
void nme_assert_handleFailure(const char* expr, const char* file,
                              const char* func, i32 line);
}  // namespace nme::de

#define NME_IMPL_CHECK(expr)                                                        \
do {                                                                                \
    if (NME_UNLIKELY(!(expr))) {                                                      \
        nme::detail::nme_assert_handleFailure(#expr, __FILE__, __func__, __LINE__); \
        NME_DEBUG_BREAK();                                                           \
    }                                                                               \
} while(0)

// Verify is always active
#define NME_VERIFY(expr) NME_IMPL_CHECK(expr)

#if NME_DEBUG
    #define NME_ASSERT(expr) NME_IMPL_CHECK(expr)
#else
    #define NME_ASSERT(expr)   ((void)sizeof((expr) ? 1 : 0))
#endif