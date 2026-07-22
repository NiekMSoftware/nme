#ifndef NME_PLATFORM_FILESYS_PATH_H_
#define NME_PLATFORM_FILESYS_PATH_H_

#include "nme/platform/types.h"

namespace nme::fs {

inline constexpr usize kMaxPath = 1024;

// Non-owning slice into an existing path string
struct StrView {
    const char* data;
    usize       len;
};

// Fixed-capacity path buffer. No heap for the common case.
struct Path {
    char  data[kMaxPath];
    usize len;
};

}  // namespace nme::fs

#endif  // NME_PLATFORM_FILESYS_PATH_H_
