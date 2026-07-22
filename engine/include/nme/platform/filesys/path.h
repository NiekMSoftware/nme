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

// Determines if the given path is an absolute path.
bool path_is_absolute(const char* path);

// Normalizes the given input path string and stores the resulting normalized path
// into the provided `Path` structure.
bool path_normalize(Path* out, const char* in);

// Joins two path strings `a` and `b` into a single normalized path and stores
// the result in the provided `Path` structure.
bool path_join(Path* out, const char* a, const char* b);

// Extracts the filename portion of the given path string.
StrView path_filename(const char* path);

// Extracts the file extension from the given path string.
StrView path_extension(const char* path);   // "a/b/c.png" -> "png"

// Extracts the stem portion of the given path string.
StrView path_stem(const char* path);

}  // namespace nme::fs

#endif  // NME_PLATFORM_FILESYS_PATH_H_
