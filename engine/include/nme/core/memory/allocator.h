//
// Created by niek on 7/18/2026.
//

#ifndef NME_ALLOCATOR_H_
#define NME_ALLOCATOR_H_

#include "nme/core/memory/align.h"

namespace nme {

struct Allocator {
    void* (*alloc)(void* self, usize bytes, usize align);
    void* (*free) (void* self, void* p, usize bytes);

    void* self;
};

inline void* alloc(const Allocator* a, const usize bytes, const usize align) {
    return a->alloc(a->self, bytes, align);
}

inline void free(const Allocator* a, void* p, const usize bytes) {
    a->free(a->self, p, bytes);
}

}  // namespace nme

#endif  // NME_ALLOCATOR_H_
