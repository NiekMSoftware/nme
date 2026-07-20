//
// Created by niek on 7/18/2026.
//

#ifndef NME_ALIGN_H_
#define NME_ALIGN_H_
#include "../../platform/debug/assert.h"
#include "nme/platform/types.h"

namespace nme {

inline uptr align_up(const uptr addr, const usize align) {
    NME_ASSERT(align != 0 && (align & (align - 1)) == 0);   // power of two
    const usize mask = align - 1;
    return (addr + mask) & ~static_cast<uptr>(mask);
}

}  // namespace nme

#endif  // NME_ALIGN_H_
